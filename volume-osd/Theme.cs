using System;
using System.Windows.Media;
using Microsoft.Win32;

namespace VolumeOsd;

/// <summary>Reads the Windows light/dark mode and accent color so the OSD matches the system flyouts.</summary>
public static class Theme
{
    public static bool IsLight()
    {
        using var key = Registry.CurrentUser.OpenSubKey(@"Software\Microsoft\Windows\CurrentVersion\Themes\Personalize");
        return key?.GetValue("AppsUseLightTheme") is int value && value != 0;
    }

    public static Color Accent(bool light)
    {
        // AccentPalette: 8 RGBA entries, Light3..Light1, Base, Dark1..Dark3. Windows 11 uses
        // Light2 on dark surfaces and Dark1 on light ones.
        try
        {
            using var key = Registry.CurrentUser.OpenSubKey(@"Software\Microsoft\Windows\CurrentVersion\Explorer\Accent");
            if (key?.GetValue("AccentPalette") is byte[] palette && palette.Length >= 32)
            {
                int i = (light ? 4 : 1) * 4;
                return Color.FromRgb(palette[i], palette[i + 1], palette[i + 2]);
            }
        }
        catch (Exception)
        {
        }

        return light ? Color.FromRgb(0x00, 0x5F, 0xB8) : Color.FromRgb(0x60, 0xCD, 0xFF);
    }
}
