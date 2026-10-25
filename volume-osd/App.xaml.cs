using System;
using System.Diagnostics;
using System.Drawing;
using System.Threading;
using System.Windows;
using Forms = System.Windows.Forms;

namespace VolumeOsd;

public partial class App : Application
{
    Mutex? instanceMutex;
    Forms.NotifyIcon? tray;
    AudioMonitor? monitor;
    OsdWindow? osd;
    Settings settings = new();
    bool launchedByPlugin;

    protected override void OnStartup(StartupEventArgs e)
    {
        instanceMutex = new Mutex(true, "VolumeOsd.SingleInstance", out bool isFirstInstance);
        if (!isFirstInstance)
        {
            Shutdown();
            return;
        }

        base.OnStartup(e);

        settings = Settings.Load();
        osd = new OsdWindow { Position = settings.Position, HideDelayMs = settings.HideDelayMs };

        WatchParent(e.Args);
        tray = CreateTray();

        monitor = new AudioMonitor();
        monitor.VolumeChanged += change =>
        {
            if (change.IsMaster && !settings.ShowMaster)
            {
                return;
            }

            Dispatcher.InvokeAsync(() => osd.ShowChange(change));
        };

        if (Array.IndexOf(e.Args, "--test") >= 0)
        {
            ShowTest();
        }
    }

    /// <summary>
    /// "--parent PID": started by the MIDI Mixer plugin, so exit together with it.
    /// </summary>
    void WatchParent(string[] args)
    {
        int index = Array.IndexOf(args, "--parent");
        if (index < 0 || index + 1 >= args.Length || !int.TryParse(args[index + 1], out int pid))
        {
            return;
        }

        launchedByPlugin = true;

        try
        {
            var parent = Process.GetProcessById(pid);
            parent.EnableRaisingEvents = true;
            parent.Exited += (_, _) => Dispatcher.InvokeAsync(Shutdown);
        }
        catch (ArgumentException)
        {
            Shutdown(); // parent already gone
        }
    }

    Forms.NotifyIcon CreateTray()
    {
        var menu = new Forms.ContextMenuStrip();

        var positions = new Forms.ToolStripMenuItem("Позиція");
        foreach (var (position, label) in new[]
        {
            (OsdPosition.TopLeft, "Зверху зліва"),
            (OsdPosition.TopCenter, "Зверху по центру"),
            (OsdPosition.TopRight, "Зверху справа"),
            (OsdPosition.BottomLeft, "Знизу зліва"),
            (OsdPosition.BottomCenter, "Знизу по центру"),
            (OsdPosition.BottomRight, "Знизу справа"),
        })
        {
            var item = new Forms.ToolStripMenuItem(label) { Checked = settings.Position == position };
            item.Click += (_, _) =>
            {
                settings.Position = position;
                osd!.Position = position;
                settings.Save();
                foreach (Forms.ToolStripMenuItem other in positions.DropDownItems)
                {
                    other.Checked = other == item;
                }

                ShowTest();
            };
            positions.DropDownItems.Add(item);
        }

        menu.Items.Add(positions);

        var master = new Forms.ToolStripMenuItem("Показувати загальну гучність") { Checked = settings.ShowMaster };
        master.Click += (_, _) =>
        {
            settings.ShowMaster = !settings.ShowMaster;
            master.Checked = settings.ShowMaster;
            settings.Save();
        };
        menu.Items.Add(master);

        menu.Items.Add("Показати тест", null, (_, _) => ShowTest());

        // When MIDI Mixer starts us through the plugin, autostart would only start a second copy
        if (!launchedByPlugin)
        {
            var autostart = new Forms.ToolStripMenuItem("Запускати з Windows") { Checked = Settings.StartWithWindows };
            autostart.Click += (_, _) =>
            {
                Settings.StartWithWindows = !Settings.StartWithWindows;
                autostart.Checked = Settings.StartWithWindows;
            };
            menu.Items.Add(autostart);
        }

        menu.Items.Add(new Forms.ToolStripSeparator());
        menu.Items.Add("Вихід", null, (_, _) => Shutdown());

        return new Forms.NotifyIcon
        {
            Icon = Icon.ExtractAssociatedIcon(Environment.ProcessPath!) ?? SystemIcons.Application,
            Text = "Volume OSD",
            ContextMenuStrip = menu,
            Visible = true,
        };
    }

    void ShowTest()
    {
        osd!.ShowChange(new VolumeChange("test", "Volume OSD", null, 0.65f, false, true));
    }

    protected override void OnExit(ExitEventArgs e)
    {
        monitor?.Dispose();

        if (tray != null)
        {
            tray.Visible = false;
            tray.Dispose();
        }

        instanceMutex?.Dispose();
        base.OnExit(e);
    }
}
