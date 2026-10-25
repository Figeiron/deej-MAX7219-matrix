using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using NAudio.CoreAudioApi;

namespace VolumeOsd;

public sealed record VolumeChange(string Key, string Name, ImageSource? Icon, float Volume, bool Muted, bool IsMaster);

/// <summary>
/// Watches the default output device and every app's audio session, and reports volume/mute changes.
/// Polling (instead of COM event callbacks) keeps it simple and survives apps and devices coming and going.
/// </summary>
public sealed class AudioMonitor : IDisposable
{
    const int PollMs = 40;
    const int RefreshMs = 1000;
    const float VolumeEpsilon = 0.002f;

    public event Action<VolumeChange>? VolumeChanged;

    sealed class Tracked
    {
        public required AudioSessionControl Session;
        public float Volume;
        public bool Muted;
        public AppInfo? Info;
    }

    sealed record AppInfo(string Key, string Name, ImageSource? Icon);

    readonly Thread thread;
    readonly Dictionary<string, ImageSource?> iconCache = new(StringComparer.OrdinalIgnoreCase);
    volatile bool running = true;

    public AudioMonitor()
    {
        thread = new Thread(Run) { IsBackground = true, Name = "AudioMonitor" };
        thread.SetApartmentState(ApartmentState.MTA);
        thread.Start();
    }

    void Run()
    {
        using var enumerator = new MMDeviceEnumerator();
        var sessions = new Dictionary<string, Tracked>();
        MMDevice? device = null;
        float masterVolume = 0;
        bool masterMuted = false;
        long nextRefresh = 0;

        while (running)
        {
            try
            {
                long now = Environment.TickCount64;
                if (device == null || now >= nextRefresh)
                {
                    nextRefresh = now + RefreshMs;
                    var current = enumerator.GetDefaultAudioEndpoint(DataFlow.Render, Role.Multimedia);

                    if (device == null || device.ID != current.ID)
                    {
                        // New output device: take fresh baselines without showing anything
                        DisposeAll(sessions);
                        device?.Dispose();
                        device = current;
                        masterVolume = device.AudioEndpointVolume.MasterVolumeLevelScalar;
                        masterMuted = device.AudioEndpointVolume.Mute;
                    }
                    else
                    {
                        current.Dispose();
                    }

                    RefreshSessions(device, sessions);
                }

                var endpoint = device.AudioEndpointVolume;
                float volume = endpoint.MasterVolumeLevelScalar;
                bool muted = endpoint.Mute;
                if (Math.Abs(volume - masterVolume) > VolumeEpsilon || muted != masterMuted)
                {
                    masterVolume = volume;
                    masterMuted = muted;
                    VolumeChanged?.Invoke(new VolumeChange("master", "Загальна гучність", null, volume, muted, true));
                }

                foreach (var tracked in sessions.Values)
                {
                    PollSession(tracked);
                }
            }
            catch (Exception)
            {
                // Device unplugged or audio service restarted: start over on the next tick
                DisposeAll(sessions);
                device?.Dispose();
                device = null;
                Thread.Sleep(500);
            }

            Thread.Sleep(PollMs);
        }

        DisposeAll(sessions);
        device?.Dispose();
    }

    void PollSession(Tracked tracked)
    {
        float volume;
        bool muted;

        try
        {
            volume = tracked.Session.SimpleAudioVolume.Volume;
            muted = tracked.Session.SimpleAudioVolume.Mute;
        }
        catch (Exception)
        {
            return;
        }

        if (Math.Abs(volume - tracked.Volume) <= VolumeEpsilon && muted == tracked.Muted)
        {
            return;
        }

        tracked.Volume = volume;
        tracked.Muted = muted;
        tracked.Info ??= Resolve(tracked.Session);
        VolumeChanged?.Invoke(new VolumeChange(tracked.Info.Key, tracked.Info.Name, tracked.Info.Icon, volume, muted, false));
    }

    static void RefreshSessions(MMDevice device, Dictionary<string, Tracked> sessions)
    {
        var manager = device.AudioSessionManager;
        manager.RefreshSessions();
        var collection = manager.Sessions;
        var seen = new HashSet<string>();

        for (int i = 0; i < collection.Count; i++)
        {
            var session = collection[i];
            string id;

            try
            {
                if (session.State == NAudio.CoreAudioApi.Interfaces.AudioSessionState.AudioSessionStateExpired)
                {
                    session.Dispose();
                    continue;
                }

                id = session.GetSessionInstanceIdentifier;
            }
            catch (Exception)
            {
                session.Dispose();
                continue;
            }

            seen.Add(id);

            if (sessions.ContainsKey(id))
            {
                session.Dispose(); // already tracking it, keep the old object and its baseline
                continue;
            }

            // New session: record its current volume silently, only later changes show the OSD
            sessions[id] = new Tracked
            {
                Session = session,
                Volume = session.SimpleAudioVolume.Volume,
                Muted = session.SimpleAudioVolume.Mute,
            };
        }

        foreach (var id in new List<string>(sessions.Keys))
        {
            if (!seen.Contains(id))
            {
                sessions[id].Session.Dispose();
                sessions.Remove(id);
            }
        }
    }

    static void DisposeAll(Dictionary<string, Tracked> sessions)
    {
        foreach (var tracked in sessions.Values)
        {
            tracked.Session.Dispose();
        }

        sessions.Clear();
    }

    AppInfo Resolve(AudioSessionControl session)
    {
        if (session.IsSystemSoundsSession)
        {
            return new AppInfo("system", "Системні звуки", null);
        }

        uint pid = session.GetProcessID;
        string? path = GetProcessPath(pid);
        string name = session.DisplayName ?? "";

        if (name.StartsWith('@')) // unresolved resource string like "@%SystemRoot%\...,-123"
        {
            name = "";
        }

        if (name.Length == 0 && path != null)
        {
            try
            {
                name = FileVersionInfo.GetVersionInfo(path).FileDescription ?? "";
            }
            catch (Exception)
            {
            }
        }

        if (name.Length == 0 && path != null)
        {
            name = Path.GetFileNameWithoutExtension(path);
        }

        if (name.Length == 0)
        {
            name = "Програма";
        }

        return new AppInfo(path ?? $"pid:{pid}", name.Trim(), path != null ? LoadIcon(path) : null);
    }

    ImageSource? LoadIcon(string path)
    {
        if (iconCache.TryGetValue(path, out var cached))
        {
            return cached;
        }

        ImageSource? source = null;

        try
        {
            using var icon = System.Drawing.Icon.ExtractAssociatedIcon(path);
            if (icon != null)
            {
                var bitmap = Imaging.CreateBitmapSourceFromHIcon(icon.Handle, Int32Rect.Empty, BitmapSizeOptions.FromEmptyOptions());
                bitmap.Freeze(); // created on the monitor thread, used on the UI thread
                source = bitmap;
            }
        }
        catch (Exception)
        {
        }

        iconCache[path] = source;
        return source;
    }

    const uint PROCESS_QUERY_LIMITED_INFORMATION = 0x1000;

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern IntPtr OpenProcess(uint access, bool inheritHandle, uint processId);

    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    static extern bool QueryFullProcessImageName(IntPtr process, uint flags, StringBuilder buffer, ref uint size);

    [DllImport("kernel32.dll")]
    static extern bool CloseHandle(IntPtr handle);

    // Works for elevated processes too, unlike Process.MainModule
    static string? GetProcessPath(uint pid)
    {
        IntPtr handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid);
        if (handle == IntPtr.Zero)
        {
            return null;
        }

        try
        {
            var buffer = new StringBuilder(1024);
            uint size = (uint)buffer.Capacity;
            return QueryFullProcessImageName(handle, 0, buffer, ref size) ? buffer.ToString() : null;
        }
        finally
        {
            CloseHandle(handle);
        }
    }

    public void Dispose()
    {
        running = false;
        thread.Join(1000);
    }
}
