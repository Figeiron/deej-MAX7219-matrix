using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows;
using System.Windows.Media;

namespace VolumeOsd;

public sealed class OsdRow : INotifyPropertyChanged
{
    public string Key { get; }

    string name = "";
    ImageSource? icon;
    double volume;
    bool muted;
    bool isMaster;

    public OsdRow(string key)
    {
        Key = key;
    }

    public string Name { get => name; private set => Set(ref name, value); }
    public ImageSource? Icon { get => icon; private set => Set(ref icon, value, nameof(IconVisibility), nameof(GlyphVisibility)); }
    public double Volume { get => volume; private set => Set(ref volume, value, nameof(PercentText)); }
    public bool Muted { get => muted; private set => Set(ref muted, value, nameof(PercentVisibility), nameof(MuteVisibility)); }

    public string PercentText => Math.Round(volume * 100).ToString();
    public Visibility IconVisibility => icon != null ? Visibility.Visible : Visibility.Collapsed;
    public Visibility GlyphVisibility => icon == null ? Visibility.Visible : Visibility.Collapsed;
    public Visibility PercentVisibility => muted ? Visibility.Collapsed : Visibility.Visible;
    public Visibility MuteVisibility => muted ? Visibility.Visible : Visibility.Collapsed;

    // Segoe Fluent Icons / MDL2: Volume, Mute, and a generic app tile for apps without an icon
    public string Glyph => muted && isMaster ? "" : isMaster || Key == "system" ? "" : "";

    public void Update(VolumeChange change)
    {
        isMaster = change.IsMaster;
        Name = change.Name;
        Icon = change.Icon;
        Volume = change.Volume;
        Muted = change.Muted;
        OnPropertyChanged(nameof(Glyph));
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    void OnPropertyChanged(string propertyName) => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));

    void Set<T>(ref T field, T value, string? also1 = null, string? also2 = null, [CallerMemberName] string? propertyName = null)
    {
        if (Equals(field, value))
        {
            return;
        }

        field = value;
        OnPropertyChanged(propertyName!);
        if (also1 != null)
        {
            OnPropertyChanged(also1);
        }

        if (also2 != null)
        {
            OnPropertyChanged(also2);
        }
    }
}
