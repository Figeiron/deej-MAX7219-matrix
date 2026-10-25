using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Threading;

namespace VolumeOsd;

public partial class OsdWindow : Window
{
    const int MaxRows = 6;
    const double ScreenMargin = 28;

    readonly ObservableCollection<OsdRow> rows = new();
    readonly DispatcherTimer hideTimer = new();
    bool fadingOut;

    public OsdPosition Position { get; set; } = OsdPosition.BottomCenter;
    public int HideDelayMs { get; set; } = 1500;

    public OsdWindow()
    {
        InitializeComponent();
        Rows.ItemsSource = rows;
        hideTimer.Tick += (_, _) =>
        {
            hideTimer.Stop();
            FadeOut();
        };
        SizeChanged += (_, _) => Reposition();
    }

    public void ShowChange(VolumeChange change)
    {
        var row = rows.FirstOrDefault(r => r.Key == change.Key);
        if (row == null)
        {
            if (rows.Count >= MaxRows)
            {
                rows.RemoveAt(0);
            }

            row = new OsdRow(change.Key);
            rows.Add(row);
        }

        row.Update(change);

        if (!IsVisible || fadingOut)
        {
            FadeIn();
        }

        hideTimer.Stop();
        hideTimer.Interval = TimeSpan.FromMilliseconds(HideDelayMs);
        hideTimer.Start();
    }

    void FadeIn()
    {
        fadingOut = false;

        if (!IsVisible)
        {
            ApplyTheme();
            Show();
            Reposition();
        }

        // Re-assert topmost so we stay above windows that grabbed it since last time
        Topmost = false;
        Topmost = true;
        BeginAnimation(OpacityProperty, new DoubleAnimation(1, TimeSpan.FromMilliseconds(120)));
    }

    void FadeOut()
    {
        fadingOut = true;
        var animation = new DoubleAnimation(0, TimeSpan.FromMilliseconds(250));
        animation.Completed += (_, _) =>
        {
            if (!fadingOut)
            {
                return; // a new change arrived while fading
            }

            fadingOut = false;
            Hide();
            rows.Clear();
        };
        BeginAnimation(OpacityProperty, animation);
    }

    void Reposition()
    {
        var area = SystemParameters.WorkArea;
        double width = ActualWidth;
        double height = ActualHeight;

        Left = Position switch
        {
            OsdPosition.TopLeft or OsdPosition.BottomLeft => area.Left + ScreenMargin,
            OsdPosition.TopRight or OsdPosition.BottomRight => area.Right - width - ScreenMargin,
            _ => area.Left + (area.Width - width) / 2,
        };

        Top = Position switch
        {
            OsdPosition.TopLeft or OsdPosition.TopCenter or OsdPosition.TopRight => area.Top + ScreenMargin,
            _ => area.Bottom - height - ScreenMargin,
        };
    }

    void ApplyTheme()
    {
        bool light = Theme.IsLight();
        SetBrush("OsdBackground", light ? "#F2F9F9F9" : "#F22B2B2B");
        SetBrush("OsdBorder", light ? "#1A000000" : "#26FFFFFF");
        SetBrush("OsdForeground", light ? "#E4000000" : "#FFFFFFFF");
        SetBrush("OsdSecondary", light ? "#9E000000" : "#C5FFFFFF");
        SetBrush("OsdTrack", light ? "#29000000" : "#40FFFFFF");
        SetBrush("OsdMuted", light ? "#73000000" : "#73FFFFFF");
        Resources["OsdAccent"] = new SolidColorBrush(Theme.Accent(light));
    }

    void SetBrush(string key, string color)
    {
        Resources[key] = new SolidColorBrush((Color)ColorConverter.ConvertFromString(color));
    }

    const int GWL_EXSTYLE = -20;
    const int WS_EX_TRANSPARENT = 0x00000020;
    const int WS_EX_TOOLWINDOW = 0x00000080;
    const int WS_EX_NOACTIVATE = 0x08000000;

    [DllImport("user32.dll")]
    static extern int GetWindowLong(IntPtr hwnd, int index);

    [DllImport("user32.dll")]
    static extern int SetWindowLong(IntPtr hwnd, int index, int value);

    protected override void OnSourceInitialized(EventArgs e)
    {
        base.OnSourceInitialized(e);

        // Click-through, never steals focus, hidden from Alt+Tab
        var hwnd = new WindowInteropHelper(this).Handle;
        int style = GetWindowLong(hwnd, GWL_EXSTYLE);
        SetWindowLong(hwnd, GWL_EXSTYLE, style | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE);
    }
}
