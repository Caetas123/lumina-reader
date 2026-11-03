using System.Diagnostics;
using System.Drawing.Drawing2D;
using System.Globalization;
using System.IO.Compression;
using Microsoft.Win32;
using System.Reflection;
using System.Windows.Forms;

internal static class Program
{
    [STAThread]
    private static void Main()
    {
        ApplicationConfiguration.Initialize();
        Application.Run(new InstallerForm());
    }
}

internal sealed class InstallerForm : Form
{
    private static readonly string ApplicationVersion =
        Assembly.GetEntryAssembly()?
            .GetCustomAttribute<AssemblyInformationalVersionAttribute>()?
            .InformationalVersion
        ?? "unknown";
    private static readonly string InstallerProductVersion = ApplicationVersion.Split('+')[0];
    private const string UninstallerFileName = "LuminaReaderUninstaller.exe";
    private const string UninstallRegistryPath =
        @"Software\Microsoft\Windows\CurrentVersion\Uninstall\LuminaReader";
    private static readonly Color Accent = Color.FromArgb(79, 70, 229);
    private static readonly Color AccentDark = Color.FromArgb(55, 48, 163);
    private static readonly Color TextPrimary = Color.FromArgb(32, 36, 43);
    private static readonly Color TextSecondary = Color.FromArgb(95, 104, 117);
    private static readonly Color SurfaceMuted = Color.FromArgb(247, 248, 250);
    private static readonly Color Border = Color.FromArgb(217, 222, 231);

    private string installRoot = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "Programs",
        "Lumina Reader");

    private readonly InstallerStrings text;
    private string executablePath;
    private readonly Image? logoImage;
    private readonly Panel contentPanel = new();
    private readonly FlowLayoutPanel footerButtons = new();

    private Button? primaryButton;
    private Button? secondaryButton;
    private Button? closeButton;
    private ProgressIndicator? progressBar;
    private Label? progressStatus;
    private Label? progressPercent;
    private TextBox? installPathTextBox;
    private CheckBox? startMenuShortcutCheckBox;
    private CheckBox? desktopShortcutCheckBox;
    private bool installationInProgress;

    public InstallerForm()
    {
        text = InstallerStrings.ForSystemLanguage();
        executablePath = Path.Combine(installRoot, "lumina_reader.exe");
        logoImage = LoadLogo();

        Text = text.SetupTitle;
        try
        {
            Icon = System.Drawing.Icon.ExtractAssociatedIcon(
                Environment.ProcessPath ?? Application.ExecutablePath);
        }
        catch
        {
            // The installer remains usable when the system cannot read its icon.
        }
        StartPosition = FormStartPosition.CenterScreen;
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false;
        MinimizeBox = false;
        ClientSize = new Size(900, 580);
        BackColor = Color.FromArgb(247, 248, 252);
        Font = new Font("Segoe UI", 9F, FontStyle.Regular, GraphicsUnit.Point);

        BuildShell();
        ShowWelcomePage();
    }

    protected override void OnFormClosing(FormClosingEventArgs e)
    {
        if (installationInProgress)
        {
            e.Cancel = true;
            return;
        }

        base.OnFormClosing(e);
    }

    private void BuildShell()
    {
        var shell = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 2,
            RowCount = 1,
            BackColor = Color.FromArgb(247, 248, 252),
            Margin = Padding.Empty,
            Padding = Padding.Empty
        };
        shell.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 278));
        shell.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        Controls.Add(shell);

        shell.Controls.Add(BuildBrandPanel(), 0, 0);

        var main = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 1,
            RowCount = 2,
            BackColor = Color.FromArgb(247, 248, 252),
            Margin = Padding.Empty,
            Padding = Padding.Empty
        };
        main.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        main.RowStyles.Add(new RowStyle(SizeType.Absolute, 82));
        shell.Controls.Add(main, 1, 0);

        contentPanel.Dock = DockStyle.Fill;
        contentPanel.BackColor = Color.FromArgb(247, 248, 252);
        contentPanel.Padding = new Padding(52, 48, 52, 24);
        main.Controls.Add(contentPanel, 0, 0);

        var footer = new Panel
        {
            Dock = DockStyle.Fill,
            BackColor = Color.FromArgb(247, 248, 252),
            Padding = new Padding(52, 16, 52, 22)
        };
        footer.Paint += (_, eventArgs) =>
        {
            using var pen = new Pen(Color.FromArgb(229, 231, 240));
            eventArgs.Graphics.DrawLine(pen, 0, 0, footer.Width, 0);
        };

        footerButtons.Dock = DockStyle.Right;
        footerButtons.AutoSize = true;
        footerButtons.AutoSizeMode = AutoSizeMode.GrowAndShrink;
        footerButtons.FlowDirection = FlowDirection.LeftToRight;
        footerButtons.WrapContents = false;
        footerButtons.BackColor = Color.Transparent;
        footer.Controls.Add(footerButtons);
        main.Controls.Add(footer, 0, 1);
    }

    private Control BuildBrandPanel()
    {
        var brand = new BrandPanel
        {
            Dock = DockStyle.Fill,
            BackColor = Color.FromArgb(31, 27, 84)
        };

        var logo = new PictureBox
        {
            Location = new Point(53, 68),
            Size = new Size(172, 172),
            SizeMode = PictureBoxSizeMode.Zoom,
            BackColor = Color.Transparent,
            Image = logoImage
        };
        brand.Controls.Add(logo);

        var title = new Label
        {
            AutoSize = true,
            Location = new Point(34, 270),
            Text = "Lumina Reader",
            Font = new Font("Segoe UI", 22F, FontStyle.Bold, GraphicsUnit.Point),
            ForeColor = Color.White,
            BackColor = Color.Transparent
        };
        brand.Controls.Add(title);

        var tagline = new Label
        {
            AutoSize = false,
            Location = new Point(36, 320),
            Size = new Size(204, 62),
            Text = text.BrandTagline,
            Font = new Font("Segoe UI", 11F, FontStyle.Regular, GraphicsUnit.Point),
            ForeColor = Color.FromArgb(226, 232, 255),
            BackColor = Color.Transparent
        };
        brand.Controls.Add(tagline);

        var version = new Label
        {
            AutoSize = false,
            Location = new Point(36, 0),
            Size = new Size(204, 24),
            Anchor = AnchorStyles.Left | AnchorStyles.Bottom,
            Text = string.Format(text.BrandVersionTemplate, ApplicationVersion),
            Font = new Font("Segoe UI", 8.5F, FontStyle.Regular, GraphicsUnit.Point),
            ForeColor = Color.FromArgb(196, 200, 239),
            BackColor = Color.Transparent
        };
        brand.Resize += (_, _) => version.Location = new Point(36, Math.Max(0, brand.ClientSize.Height - 48));
        brand.Controls.Add(version);

        return brand;
    }

    private sealed class BrandPanel : Panel
    {
        public BrandPanel()
        {
            DoubleBuffered = true;
            SetStyle(ControlStyles.SupportsTransparentBackColor, true);
        }

        protected override void OnPaintBackground(PaintEventArgs eventArgs)
        {
            using var brush = new LinearGradientBrush(
                ClientRectangle,
                Color.FromArgb(27, 24, 74),
                Color.FromArgb(79, 70, 229),
                45F);
            eventArgs.Graphics.FillRectangle(brush, ClientRectangle);

            using var glowBrush = new SolidBrush(Color.FromArgb(35, 255, 255, 255));
            eventArgs.Graphics.FillEllipse(glowBrush, new Rectangle(150, 250, 210, 210));
        }
    }

    private sealed class ProgressIndicator : Control
    {
        private int value;

        [System.ComponentModel.DesignerSerializationVisibility(System.ComponentModel.DesignerSerializationVisibility.Hidden)]
        public int Value
        {
            get => value;
            set
            {
                this.value = Math.Clamp(value, 0, 100);
                Invalidate();
            }
        }

        public ProgressIndicator()
        {
            DoubleBuffered = true;
            SetStyle(
                ControlStyles.UserPaint |
                ControlStyles.AllPaintingInWmPaint |
                ControlStyles.OptimizedDoubleBuffer |
                ControlStyles.SupportsTransparentBackColor,
                true);
            BackColor = Color.Transparent;
        }

        protected override void OnPaint(PaintEventArgs eventArgs)
        {
            base.OnPaint(eventArgs);
            if (Width <= 0 || Height <= 0)
            {
                return;
            }

            eventArgs.Graphics.SmoothingMode = SmoothingMode.AntiAlias;
            var bounds = new Rectangle(0, 0, Width - 1, Height - 1);
            using var trackPath = CreateRoundedPath(bounds, Height - 1);
            using var trackBrush = new SolidBrush(Color.FromArgb(229, 231, 240));
            eventArgs.Graphics.FillPath(trackBrush, trackPath);

            if (value == 0)
            {
                return;
            }

            var fillWidth = Math.Max(Height, (int)Math.Round(bounds.Width * value / 100D));
            eventArgs.Graphics.SetClip(trackPath);
            using var fillBrush = new LinearGradientBrush(
                new Rectangle(0, 0, Math.Max(1, fillWidth), Height),
                Accent,
                Color.FromArgb(124, 92, 246),
                0F);
            eventArgs.Graphics.FillRectangle(fillBrush, 0, 0, fillWidth, Height);
            eventArgs.Graphics.ResetClip();
        }

        private static GraphicsPath CreateRoundedPath(Rectangle bounds, int radius)
        {
            var diameter = Math.Max(1, radius);
            var path = new GraphicsPath();
            path.AddArc(bounds.X, bounds.Y, diameter, diameter, 180, 90);
            path.AddArc(bounds.Right - diameter, bounds.Y, diameter, diameter, 270, 90);
            path.AddArc(bounds.Right - diameter, bounds.Bottom - diameter, diameter, diameter, 0, 90);
            path.AddArc(bounds.X, bounds.Bottom - diameter, diameter, diameter, 90, 90);
            path.CloseFigure();
            return path;
        }
    }

    private static Image? LoadLogo()
    {
        var resourceName = Assembly.GetExecutingAssembly()
            .GetManifestResourceNames()
            .SingleOrDefault(name => name.EndsWith("lumina-logo.png", StringComparison.OrdinalIgnoreCase));
        if (resourceName is null)
        {
            return null;
        }

        using var stream = Assembly.GetExecutingAssembly().GetManifestResourceStream(resourceName);
        if (stream is null)
        {
            return null;
        }

        using var sourceImage = Image.FromStream(stream);
        return new Bitmap(sourceImage);
    }

    private void ShowWelcomePage()
    {
        installationInProgress = false;
        ClearContent();
        ConfigureFooter();

        var layout = CreateContentLayout();
        layout.Controls.Add(CreateHeading(text.WelcomeHeading));
        layout.Controls.Add(CreateBody(text.WelcomeBody));

        layout.Controls.Add(CreateInfoCard(
            text.WhatYouAreInstallingHeading,
            text.WhatYouAreInstallingBody));

        layout.Controls.Add(CreateMutedNote(text.PhaseNote));

        closeButton = CreateButton(text.Close, isPrimary: false);
        closeButton.Click += (_, _) => Close();
        primaryButton = CreateButton(text.Continue, isPrimary: true);
        primaryButton.Click += (_, _) => ShowConfirmationPage();
        footerButtons.Controls.Add(closeButton);
        footerButtons.Controls.Add(primaryButton);
    }

    private void ShowConfirmationPage()
    {
        ClearContent();
        ConfigureFooter();

        var layout = CreateContentLayout();
        layout.Controls.Add(CreateHeading(text.ReadyToInstallHeading));
        layout.Controls.Add(CreateBody(text.ReadyToInstallBody));

        layout.Controls.Add(CreateInstallOptionsPanel());
        layout.Controls.Add(CreateMutedNote(text.ConfirmNote));

        secondaryButton = CreateButton(text.Back, isPrimary: false);
        secondaryButton.Click += (_, _) => ShowWelcomePage();
        closeButton = CreateButton(text.Close, isPrimary: false);
        closeButton.Click += (_, _) => Close();
        primaryButton = CreateButton(text.Install, isPrimary: true);
        primaryButton.Click += async (_, _) => await BeginInstallationAsync();
        footerButtons.Controls.Add(secondaryButton);
        footerButtons.Controls.Add(closeButton);
        footerButtons.Controls.Add(primaryButton);
    }

    private void ShowProgressPage()
    {
        ClearContent();
        ConfigureFooter();

        var layout = CreateContentLayout();
        layout.Controls.Add(CreateHeading(text.InstallingHeading));
        layout.Controls.Add(CreateBody(text.InstallingBody));

        progressStatus = new Label
        {
            AutoSize = false,
            Height = 28,
            Dock = DockStyle.Top,
            Text = text.PreparingInstallation,
            ForeColor = TextPrimary,
            Font = new Font("Segoe UI", 10F, FontStyle.Regular, GraphicsUnit.Point),
            TextAlign = ContentAlignment.MiddleLeft,
            Margin = new Padding(0, 20, 0, 2)
        };
        layout.Controls.Add(progressStatus);

        progressBar = new ProgressIndicator
        {
            Width = contentPanel.ClientSize.Width - contentPanel.Padding.Horizontal,
            Height = 12,
            Margin = new Padding(0, 0, 0, 0)
        };
        layout.Controls.Add(progressBar);

        progressPercent = new Label
        {
            AutoSize = false,
            Height = 28,
            Dock = DockStyle.Top,
            Text = "0%",
            ForeColor = TextSecondary,
            TextAlign = ContentAlignment.MiddleRight,
            Margin = new Padding(0, 4, 0, 0)
        };
        layout.Controls.Add(progressPercent);

        footerButtons.Controls.Add(CreateButton(text.InstallingButton, isPrimary: true, enabled: false));
    }

    private void ShowCompletePage()
    {
        installationInProgress = false;
        ClearContent();
        ConfigureFooter();

        var layout = CreateContentLayout();
        layout.Controls.Add(CreateHeading(text.InstallationCompleteHeading));
        layout.Controls.Add(CreateBody(text.InstallationCompleteBody));
        layout.Controls.Add(CreateInfoCard(text.InstalledToHeading, installRoot));
        layout.Controls.Add(CreateMutedNote(text.InstallationCompleteNote));

        closeButton = CreateButton(text.Close, isPrimary: false);
        closeButton.Click += (_, _) => Close();
        primaryButton = CreateButton(text.Launch, isPrimary: true);
        primaryButton.Click += (_, _) =>
        {
            if (LaunchApplication())
            {
                Close();
            }
        };
        footerButtons.Controls.Add(closeButton);
        footerButtons.Controls.Add(primaryButton);
    }

    private async Task BeginInstallationAsync()
    {
        var createStartMenuShortcut = startMenuShortcutCheckBox?.Checked ?? true;
        var createDesktopShortcut = desktopShortcutCheckBox?.Checked ?? true;
        installationInProgress = true;
        ShowProgressPage();

        var progress = new Progress<InstallProgress>(update =>
        {
            if (progressBar is not null)
            {
                progressBar.Value = Math.Clamp(update.Percent, 0, 100);
            }

            if (progressStatus is not null)
            {
                progressStatus.Text = update.Status;
            }

            if (progressPercent is not null)
            {
                progressPercent.Text = $"{update.Percent}%";
            }
        });

        try
        {
            await Task.Run(() => InstallFiles(progress, createStartMenuShortcut, createDesktopShortcut));
            ShowCompletePage();
        }
        catch (Exception exception)
        {
            installationInProgress = false;
            MessageBox.Show(
                string.Format(text.InstallationError, exception.Message),
                text.SetupTitle,
                MessageBoxButtons.OK,
                MessageBoxIcon.Error);
            ShowConfirmationPage();
        }
    }

    private void InstallFiles(
        IProgress<InstallProgress> progress,
        bool createStartMenuShortcut,
        bool createDesktopShortcut)
    {
        var resourceName = Assembly.GetExecutingAssembly()
            .GetManifestResourceNames()
            .Single(name => name.EndsWith("LuminaReader-windows-x64.zip", StringComparison.Ordinal));

        using var resourceStream = Assembly.GetExecutingAssembly().GetManifestResourceStream(resourceName)
            ?? throw new InvalidOperationException(text.PackageMissing);
        using var archive = new ZipArchive(resourceStream, ZipArchiveMode.Read);

        var files = archive.Entries.Where(entry => !string.IsNullOrEmpty(entry.Name)).ToArray();
        var totalBytes = files.Sum(entry => entry.Length);
        var completedBytes = 0L;
        var lastPercent = -1;

        Directory.CreateDirectory(installRoot);

        foreach (var entry in files)
        {
            var destinationPath = GetSafeDestinationPath(entry.FullName);
            var destinationDirectory = Path.GetDirectoryName(destinationPath);
            if (!string.IsNullOrEmpty(destinationDirectory))
            {
                Directory.CreateDirectory(destinationDirectory);
            }

            progress.Report(new InstallProgress(lastPercent < 0 ? 0 : lastPercent, string.Format(text.InstallingFileTemplate, entry.Name)));

            using var input = entry.Open();
            using var output = new FileStream(destinationPath, FileMode.Create, FileAccess.Write, FileShare.None);
            var buffer = new byte[1024 * 128];
            int read;
            while ((read = input.Read(buffer, 0, buffer.Length)) > 0)
            {
                output.Write(buffer, 0, read);
                completedBytes += read;
                var percent = totalBytes == 0 ? 100 : (int)(completedBytes * 100 / totalBytes);
                if (percent != lastPercent)
                {
                    lastPercent = percent;
                    progress.Report(new InstallProgress(percent, string.Format(text.InstallingFileTemplate, entry.Name)));
                }
            }
        }

        if (!File.Exists(executablePath))
        {
            throw new InvalidOperationException(text.ExecutableMissing);
        }

        progress.Report(new InstallProgress(100, text.RegisteringApplication));
        InstallUninstaller();
        RegisterUninstallEntry();

        if (createStartMenuShortcut || createDesktopShortcut)
        {
            progress.Report(new InstallProgress(100, text.CreatingShortcuts));
            if (createStartMenuShortcut)
            {
                CreateStartMenuShortcut();
            }

            if (createDesktopShortcut)
            {
                CreateDesktopShortcut();
            }
        }
        progress.Report(new InstallProgress(100, text.InstallationCompleteStatus));
    }

    private void InstallUninstaller()
    {
        var resourceName = Assembly.GetExecutingAssembly()
            .GetManifestResourceNames()
            .SingleOrDefault(name => name.EndsWith(UninstallerFileName, StringComparison.Ordinal));
        if (resourceName is null)
        {
            throw new InvalidOperationException(text.UninstallerMissing);
        }

        using var resourceStream = Assembly.GetExecutingAssembly().GetManifestResourceStream(resourceName)
            ?? throw new InvalidOperationException(text.UninstallerMissing);
        var destination = Path.Combine(installRoot, UninstallerFileName);
        using var output = new FileStream(destination, FileMode.Create, FileAccess.Write, FileShare.None);
        resourceStream.CopyTo(output);
    }

    private void RegisterUninstallEntry()
    {
        var uninstallerPath = Path.Combine(installRoot, UninstallerFileName);
        var estimatedSize = Directory.EnumerateFiles(installRoot, "*", SearchOption.AllDirectories)
            .Select(path => new FileInfo(path).Length)
            .Aggregate(0L, (total, length) => total + length);
        var estimatedSizeInKilobytes = (int)Math.Clamp((estimatedSize + 1023L) / 1024L, 1L, int.MaxValue);

        using var uninstallKey = Registry.CurrentUser.CreateSubKey(UninstallRegistryPath, writable: true)
            ?? throw new InvalidOperationException(text.UninstallRegistrationFailed);
        uninstallKey.SetValue("DisplayName", "Lumina Reader");
        uninstallKey.SetValue("DisplayVersion", InstallerProductVersion);
        uninstallKey.SetValue("Publisher", "Lumina Reader Project");
        uninstallKey.SetValue("InstallLocation", installRoot);
        uninstallKey.SetValue("DisplayIcon", $"{executablePath},0");
        uninstallKey.SetValue("UninstallString", $"\"{uninstallerPath}\"");
        uninstallKey.SetValue("QuietUninstallString", $"\"{uninstallerPath}\" --quiet");
        uninstallKey.SetValue("EstimatedSize", estimatedSizeInKilobytes, RegistryValueKind.DWord);
        uninstallKey.SetValue("InstallDate", DateTime.Now.ToString("yyyyMMdd", CultureInfo.InvariantCulture));
        uninstallKey.SetValue("URLInfoAbout", "https://github.com/Caetas123/lumina-reader");
        uninstallKey.SetValue("NoModify", 1, RegistryValueKind.DWord);
        uninstallKey.SetValue("NoRepair", 1, RegistryValueKind.DWord);
    }

    private string GetSafeDestinationPath(string entryName)
    {
        var root = Path.GetFullPath(installRoot + Path.DirectorySeparatorChar);
        var relativePath = entryName.Replace('/', Path.DirectorySeparatorChar);
        var destination = Path.GetFullPath(Path.Combine(installRoot, relativePath));
        if (!destination.StartsWith(root, StringComparison.OrdinalIgnoreCase))
        {
            throw new InvalidOperationException(text.InvalidPackagePath);
        }

        return destination;
    }

    private bool CreateStartMenuShortcut()
    {
        try
        {
            var startMenuRoot = Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.StartMenu),
                "Programs",
                "Lumina Reader");
            Directory.CreateDirectory(startMenuRoot);
            return TryCreateShortcut(Path.Combine(startMenuRoot, "Lumina Reader.lnk"));
        }
        catch
        {
            // Installation remains successful when shortcut creation is unavailable.
            return false;
        }
    }

    private bool CreateDesktopShortcut()
    {
        try
        {
            var desktopPath = Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory);
            return TryCreateShortcut(Path.Combine(desktopPath, "Lumina Reader.lnk"));
        }
        catch
        {
            // Installation remains successful when shortcut creation is unavailable.
            return false;
        }
    }

    private bool TryCreateShortcut(string shortcutPath)
    {
        var shellType = Type.GetTypeFromProgID("WScript.Shell");
        if (shellType is null)
        {
            return false;
        }

        dynamic shell = Activator.CreateInstance(shellType)!;
        dynamic shortcut = shell.CreateShortcut(shortcutPath);
        shortcut.TargetPath = executablePath;
        shortcut.WorkingDirectory = installRoot;
        shortcut.IconLocation = $"{executablePath},0";
        shortcut.Save();
        return File.Exists(shortcutPath);
    }

    private bool LaunchApplication()
    {
        if (!File.Exists(executablePath))
        {
            MessageBox.Show(
                text.ExecutableMissing,
                text.SetupTitle,
                MessageBoxButtons.OK,
                MessageBoxIcon.Error);
            return false;
        }

        try
        {
            var process = Process.Start(new ProcessStartInfo
            {
                FileName = executablePath,
                WorkingDirectory = installRoot,
                UseShellExecute = false
            });
            return process is not null;
        }
        catch (Exception exception)
        {
            MessageBox.Show(
                string.Format(text.LaunchError, exception.Message),
                text.SetupTitle,
                MessageBoxButtons.OK,
                MessageBoxIcon.Error);
            return false;
        }
    }

    private void ClearContent()
    {
        foreach (Control control in contentPanel.Controls)
        {
            control.Dispose();
        }

        contentPanel.Controls.Clear();
        footerButtons.Controls.Clear();
        primaryButton = null;
        secondaryButton = null;
        closeButton = null;
        progressBar = null;
        progressStatus = null;
        progressPercent = null;
        installPathTextBox = null;
        startMenuShortcutCheckBox = null;
        desktopShortcutCheckBox = null;
    }

    private void ConfigureFooter()
    {
        footerButtons.Controls.Clear();
        footerButtons.Enabled = true;
    }

    private FlowLayoutPanel CreateContentLayout()
    {
        var layout = new FlowLayoutPanel
        {
            Dock = DockStyle.Fill,
            FlowDirection = FlowDirection.TopDown,
            WrapContents = false,
            AutoScroll = false,
            BackColor = Color.Transparent,
            Padding = Padding.Empty,
            Margin = Padding.Empty
        };
        contentPanel.Controls.Add(layout);
        return layout;
    }

    private Label CreateHeading(string text)
    {
        return new Label
        {
            AutoSize = false,
            Width = contentPanel.ClientSize.Width - contentPanel.Padding.Horizontal,
            Height = 44,
            Text = text,
            Font = new Font("Segoe UI", 25F, FontStyle.Bold, GraphicsUnit.Point),
            ForeColor = TextPrimary,
            Margin = new Padding(0, 0, 0, 8)
        };
    }

    private Label CreateBody(string text)
    {
        return new Label
        {
            AutoSize = false,
            Width = contentPanel.ClientSize.Width - contentPanel.Padding.Horizontal,
            Height = 62,
            Text = text,
            Font = new Font("Segoe UI", 10.5F, FontStyle.Regular, GraphicsUnit.Point),
            ForeColor = TextSecondary,
            Margin = new Padding(0, 0, 0, 16)
        };
    }

    private Panel CreateInstallOptionsPanel()
    {
        var panel = new Panel
        {
            Width = contentPanel.ClientSize.Width - contentPanel.Padding.Horizontal,
            Height = 164,
            BackColor = Color.White,
            Margin = new Padding(0, 0, 0, 14),
            Padding = new Padding(16, 12, 16, 10)
        };
        panel.Paint += (_, eventArgs) =>
        {
            eventArgs.Graphics.SmoothingMode = SmoothingMode.AntiAlias;
            using var pen = new Pen(Color.FromArgb(229, 231, 240));
            eventArgs.Graphics.DrawRectangle(pen, 0, 0, panel.Width - 1, panel.Height - 1);
            using var accentBrush = new SolidBrush(Accent);
            eventArgs.Graphics.FillRectangle(accentBrush, 0, 14, 4, panel.Height - 28);
        };
        ApplyRoundedCorners(panel, 14);

        var headingLabel = new Label
        {
            AutoSize = true,
            Location = new Point(16, 11),
            Text = text.InstallationLocationHeading,
            Font = new Font("Segoe UI", 9.5F, FontStyle.Bold, GraphicsUnit.Point),
            ForeColor = TextPrimary
        };
        panel.Controls.Add(headingLabel);

        installPathTextBox = new TextBox
        {
            Location = new Point(16, 37),
            Width = panel.Width - 142,
            Height = 28,
            ReadOnly = true,
            Text = installRoot,
            BackColor = Color.White,
            BorderStyle = BorderStyle.FixedSingle,
            Font = new Font("Segoe UI", 9F, FontStyle.Regular, GraphicsUnit.Point)
        };
        panel.Controls.Add(installPathTextBox);

        var browseButton = CreateButton(text.Browse, isPrimary: false);
        browseButton.AutoSize = false;
        browseButton.Width = 102;
        browseButton.Height = 30;
        browseButton.Location = new Point(panel.Width - browseButton.Width - 16, 36);
        browseButton.Margin = Padding.Empty;
        browseButton.Click += (_, _) => ChooseInstallFolder();
        panel.Controls.Add(browseButton);

        startMenuShortcutCheckBox = new CheckBox
        {
            AutoSize = true,
            Location = new Point(16, 82),
            Text = text.StartMenuShortcut,
            Checked = true,
            ForeColor = TextPrimary,
            Font = new Font("Segoe UI", 9.5F, FontStyle.Regular, GraphicsUnit.Point),
            BackColor = Color.White
        };
        panel.Controls.Add(startMenuShortcutCheckBox);

        desktopShortcutCheckBox = new CheckBox
        {
            AutoSize = true,
            Location = new Point(16, 108),
            Text = text.DesktopShortcut,
            Checked = true,
            ForeColor = TextPrimary,
            Font = new Font("Segoe UI", 9.5F, FontStyle.Regular, GraphicsUnit.Point),
            BackColor = Color.White
        };
        panel.Controls.Add(desktopShortcutCheckBox);

        return panel;
    }

    private void ChooseInstallFolder()
    {
        using var dialog = new FolderBrowserDialog
        {
            Description = text.SelectInstallFolder,
            SelectedPath = installRoot,
            ShowNewFolderButton = true
        };

        if (dialog.ShowDialog(this) == DialogResult.OK)
        {
            installRoot = dialog.SelectedPath;
            executablePath = Path.Combine(installRoot, "lumina_reader.exe");
            if (installPathTextBox is not null)
            {
                installPathTextBox.Text = installRoot;
            }
        }
    }

    private Panel CreateInfoCard(string heading, string body)
    {
        var card = new Panel
        {
            Width = contentPanel.ClientSize.Width - contentPanel.Padding.Horizontal,
            Height = 108,
            BackColor = Color.White,
            Margin = new Padding(0, 0, 0, 14),
            Padding = new Padding(16, 12, 16, 10)
        };
        card.Paint += (_, eventArgs) =>
        {
            eventArgs.Graphics.SmoothingMode = SmoothingMode.AntiAlias;
            using var pen = new Pen(Color.FromArgb(229, 231, 240));
            eventArgs.Graphics.DrawRectangle(pen, 0, 0, card.Width - 1, card.Height - 1);
            using var accentBrush = new SolidBrush(Accent);
            eventArgs.Graphics.FillRectangle(accentBrush, 0, 14, 4, card.Height - 28);
        };
        ApplyRoundedCorners(card, 14);

        var headingLabel = new Label
        {
            AutoSize = true,
            Location = new Point(16, 11),
            Text = heading,
            Font = new Font("Segoe UI", 9.5F, FontStyle.Bold, GraphicsUnit.Point),
            ForeColor = TextPrimary
        };
        card.Controls.Add(headingLabel);

        var bodyLabel = new Label
        {
            AutoSize = false,
            Location = new Point(16, 35),
            Width = card.Width - 32,
            Height = card.Height - 43,
            Text = body,
            Font = new Font("Segoe UI", 9.5F, FontStyle.Regular, GraphicsUnit.Point),
            ForeColor = TextSecondary
        };
        card.Controls.Add(bodyLabel);
        return card;
    }

    private Label CreateMutedNote(string text)
    {
        return new Label
        {
            AutoSize = false,
            Width = contentPanel.ClientSize.Width - contentPanel.Padding.Horizontal,
            Height = 42,
            Text = text,
            Font = new Font("Segoe UI", 8.5F, FontStyle.Regular, GraphicsUnit.Point),
            ForeColor = TextSecondary,
            Margin = new Padding(0, 0, 0, 0)
        };
    }

    private Button CreateButton(string text, bool isPrimary, bool enabled = true)
    {
        var button = new RoundedButton
        {
            AutoSize = true,
            Height = 36,
            Text = text,
            Enabled = enabled,
            FlatStyle = FlatStyle.Flat,
            Font = new Font("Segoe UI", 9F, FontStyle.Regular, GraphicsUnit.Point),
            Padding = new Padding(15, 0, 15, 0),
            Margin = new Padding(10, 0, 0, 0),
            UseVisualStyleBackColor = false,
            BackColor = isPrimary ? Accent : Color.White,
            ForeColor = isPrimary ? Color.White : TextPrimary
        };
        button.FlatAppearance.BorderColor = isPrimary ? Accent : Color.FromArgb(220, 223, 234);
        button.FlatAppearance.BorderSize = isPrimary ? 0 : 1;
        button.FlatAppearance.MouseOverBackColor = isPrimary ? AccentDark : SurfaceMuted;
        button.FlatAppearance.MouseDownBackColor = isPrimary ? AccentDark : Border;
        return button;
    }

    private sealed class RoundedButton : Button
    {
        private bool isHovered;
        private bool isPressed;

        private const float CornerRadius = 9F;

        public RoundedButton()
        {
            SetStyle(
                ControlStyles.UserPaint |
                ControlStyles.AllPaintingInWmPaint |
                ControlStyles.OptimizedDoubleBuffer |
                ControlStyles.ResizeRedraw,
                true);
            FlatStyle = FlatStyle.Flat;
        }

        protected override void OnMouseEnter(EventArgs eventArgs)
        {
            base.OnMouseEnter(eventArgs);
            isHovered = true;
            Invalidate();
        }

        protected override void OnMouseLeave(EventArgs eventArgs)
        {
            base.OnMouseLeave(eventArgs);
            isHovered = false;
            isPressed = false;
            Invalidate();
        }

        protected override void OnMouseDown(MouseEventArgs eventArgs)
        {
            base.OnMouseDown(eventArgs);
            if (eventArgs.Button == MouseButtons.Left)
            {
                isPressed = true;
                Invalidate();
            }
        }

        protected override void OnMouseUp(MouseEventArgs eventArgs)
        {
            base.OnMouseUp(eventArgs);
            if (eventArgs.Button == MouseButtons.Left)
            {
                isPressed = false;
                Invalidate();
            }
        }

        protected override void OnEnabledChanged(EventArgs eventArgs)
        {
            base.OnEnabledChanged(eventArgs);
            Invalidate();
        }

        protected override void OnPaintBackground(PaintEventArgs eventArgs)
        {
            // The anti-aliased rounded surface is painted in OnPaint. Painting a
            // rectangular background here would reintroduce square corners.
        }

        protected override void OnPaint(PaintEventArgs eventArgs)
        {
            if (ClientSize.Width <= 0 || ClientSize.Height <= 0)
            {
                return;
            }

            var graphics = eventArgs.Graphics;
            graphics.SmoothingMode = SmoothingMode.AntiAlias;
            graphics.PixelOffsetMode = PixelOffsetMode.HighQuality;
            graphics.CompositingQuality = CompositingQuality.HighQuality;
            graphics.Clear(FindSurfaceColor());

            var borderSize = Math.Max(0, FlatAppearance.BorderSize);
            var inset = Math.Max(0.5F, borderSize / 2F);
            var bounds = new RectangleF(
                inset,
                inset,
                Math.Max(1F, ClientSize.Width - (inset * 2F)),
                Math.Max(1F, ClientSize.Height - (inset * 2F)));
            var radius = Math.Min(CornerRadius, Math.Min(bounds.Width, bounds.Height) / 2F);

            using var path = CreateRoundedPath(bounds, radius);
            using var fillBrush = new SolidBrush(GetFillColor());
            graphics.FillPath(fillBrush, path);

            if (borderSize > 0)
            {
                using var borderPen = new Pen(FlatAppearance.BorderColor, borderSize)
                {
                    Alignment = PenAlignment.Center
                };
                graphics.DrawPath(borderPen, path);
            }

            var textColor = Enabled ? ForeColor : Color.FromArgb(160, 164, 174);
            TextRenderer.DrawText(
                graphics,
                Text,
                Font,
                ClientRectangle,
                textColor,
                TextFormatFlags.HorizontalCenter |
                TextFormatFlags.VerticalCenter |
                TextFormatFlags.NoPadding |
                TextFormatFlags.NoPrefix);

        }

        private Color GetFillColor()
        {
            if (!Enabled)
            {
                return Color.FromArgb(235, 236, 241);
            }

            if (isPressed)
            {
                return FlatAppearance.MouseDownBackColor;
            }

            if (isHovered)
            {
                return FlatAppearance.MouseOverBackColor;
            }

            return BackColor;
        }

        private Color FindSurfaceColor()
        {
            for (Control? parent = Parent; parent is not null; parent = parent.Parent)
            {
                if (parent.BackColor.A > 0)
                {
                    return parent.BackColor;
                }
            }

            return SystemColors.Control;
        }

        private static GraphicsPath CreateRoundedPath(RectangleF bounds, float radius)
        {
            var diameter = Math.Max(1F, radius * 2F);
            var path = new GraphicsPath();
            path.AddArc(bounds.X, bounds.Y, diameter, diameter, 180F, 90F);
            path.AddArc(bounds.Right - diameter, bounds.Y, diameter, diameter, 270F, 90F);
            path.AddArc(bounds.Right - diameter, bounds.Bottom - diameter, diameter, diameter, 0F, 90F);
            path.AddArc(bounds.X, bounds.Bottom - diameter, diameter, diameter, 90F, 90F);
            path.CloseFigure();
            return path;
        }
    }

    private static void ApplyRoundedCorners(Control control, int radius)
    {
        void UpdateRegion(object? _, EventArgs __)
        {
            using var path = new GraphicsPath();
            var bounds = new Rectangle(0, 0, control.Width, control.Height);
            var diameter = radius * 2;
            path.AddArc(bounds.X, bounds.Y, diameter, diameter, 180, 90);
            path.AddArc(bounds.Right - diameter, bounds.Y, diameter, diameter, 270, 90);
            path.AddArc(bounds.Right - diameter, bounds.Bottom - diameter, diameter, diameter, 0, 90);
            path.AddArc(bounds.X, bounds.Bottom - diameter, diameter, diameter, 90, 90);
            path.CloseFigure();
            control.Region = new Region(path);
        }

        control.SizeChanged += UpdateRegion;
        UpdateRegion(null, EventArgs.Empty);
    }

    private sealed class InstallerStrings
    {
        public string SetupTitle { get; init; } = string.Empty;
        public string BrandTagline { get; init; } = string.Empty;
        public string BrandVersionTemplate { get; init; } = string.Empty;
        public string WelcomeHeading { get; init; } = string.Empty;
        public string WelcomeBody { get; init; } = string.Empty;
        public string WhatYouAreInstallingHeading { get; init; } = string.Empty;
        public string WhatYouAreInstallingBody { get; init; } = string.Empty;
        public string PhaseNote { get; init; } = string.Empty;
        public string Close { get; init; } = string.Empty;
        public string Continue { get; init; } = string.Empty;
        public string ReadyToInstallHeading { get; init; } = string.Empty;
        public string ReadyToInstallBody { get; init; } = string.Empty;
        public string InstallationLocationHeading { get; init; } = string.Empty;
        public string Browse { get; init; } = string.Empty;
        public string SelectInstallFolder { get; init; } = string.Empty;
        public string StartMenuShortcut { get; init; } = string.Empty;
        public string DesktopShortcut { get; init; } = string.Empty;
        public string ConfirmNote { get; init; } = string.Empty;
        public string Back { get; init; } = string.Empty;
        public string Install { get; init; } = string.Empty;
        public string InstallingHeading { get; init; } = string.Empty;
        public string InstallingBody { get; init; } = string.Empty;
        public string PreparingInstallation { get; init; } = string.Empty;
        public string InstallingButton { get; init; } = string.Empty;
        public string InstallationCompleteHeading { get; init; } = string.Empty;
        public string InstallationCompleteBody { get; init; } = string.Empty;
        public string InstalledToHeading { get; init; } = string.Empty;
        public string InstallationCompleteNote { get; init; } = string.Empty;
        public string Launch { get; init; } = string.Empty;
        public string InstallingFileTemplate { get; init; } = string.Empty;
        public string RegisteringApplication { get; init; } = string.Empty;
        public string CreatingShortcuts { get; init; } = string.Empty;
        public string InstallationCompleteStatus { get; init; } = string.Empty;
        public string InstallationError { get; init; } = string.Empty;
        public string LaunchError { get; init; } = string.Empty;
        public string PackageMissing { get; init; } = string.Empty;
        public string ExecutableMissing { get; init; } = string.Empty;
        public string UninstallerMissing { get; init; } = string.Empty;
        public string UninstallRegistrationFailed { get; init; } = string.Empty;
        public string InvalidPackagePath { get; init; } = string.Empty;

        public static InstallerStrings ForSystemLanguage()
        {
            var language = CultureInfo.InstalledUICulture.TwoLetterISOLanguageName;
            return language.Equals("pt", StringComparison.OrdinalIgnoreCase)
                ? Portuguese()
                : English();
        }

        private static InstallerStrings English()
        {
            return new InstallerStrings
            {
                SetupTitle = "Lumina Reader Setup",
                BrandTagline = "A calm place for your reading library.",
                BrandVersionTemplate = "Windows x64 · Version {0}",
                WelcomeHeading = "Welcome to Lumina Reader",
                WelcomeBody = "Lumina Reader gives you a calm, private place to read and organize your books and documents. Your files stay on your computer by default.",
                WhatYouAreInstallingHeading = "What you are installing",
                WhatYouAreInstallingBody = "• Read and organize your books and documents\n• Save passages, notes, and your reading progress\n• Keep your library private on your computer\n• Ready to open after installation",
                PhaseNote = "A calm reading space for your personal library. New format and study capabilities can arrive through future updates.",
                Close = "Close",
                Continue = "Continue",
                ReadyToInstallHeading = "Ready to install?",
                ReadyToInstallBody = "Choose where Lumina Reader should be installed and which shortcuts you would like to create.",
                InstallationLocationHeading = "Installation location",
                Browse = "Browse...",
                SelectInstallFolder = "Choose where to install Lumina Reader",
                StartMenuShortcut = "Create a shortcut in the Start menu",
                DesktopShortcut = "Create a shortcut on the desktop",
                ConfirmNote = "Choose Install to continue, or Close to leave without making changes.",
                Back = "Back",
                Install = "Install",
                InstallingHeading = "Installing Lumina Reader",
                InstallingBody = "Please wait while Lumina Reader is installed. This should only take a moment.",
                PreparingInstallation = "Preparing installation...",
                InstallingButton = "Installing...",
                InstallationCompleteHeading = "Installation complete",
                InstallationCompleteBody = "Lumina Reader is ready to use. Your selected shortcuts were created.",
                InstalledToHeading = "Installed to",
                InstallationCompleteNote = "Click Launch to open Lumina Reader now, or Close to finish.",
                Launch = "Launch Lumina Reader",
                InstallingFileTemplate = "Installing {0}...",
                RegisteringApplication = "Registering Lumina Reader in Windows...",
                CreatingShortcuts = "Creating shortcuts...",
                InstallationCompleteStatus = "Installation complete.",
                InstallationError = "Lumina Reader could not be installed.\n\n{0}",
                LaunchError = "Lumina Reader could not be opened.\n\n{0}",
                PackageMissing = "The application package is missing.",
                ExecutableMissing = "The application executable was not found after installation.",
                UninstallerMissing = "The uninstaller component is missing.",
                UninstallRegistrationFailed = "Lumina Reader could not be registered for Windows uninstall.",
                InvalidPackagePath = "The application package contains an invalid path."
            };
        }

        private static InstallerStrings Portuguese()
        {
            return new InstallerStrings
            {
                SetupTitle = "Instalação do Lumina Reader",
                BrandTagline = "Um espaço tranquilo para sua biblioteca de leitura.",
                BrandVersionTemplate = "Windows x64 · Versão {0}",
                WelcomeHeading = "Bem-vindo ao Lumina Reader",
                WelcomeBody = "O Lumina Reader oferece um espaço tranquilo e privado para ler e organizar seus livros e documentos. Seus arquivos ficam no seu computador por padrão.",
                WhatYouAreInstallingHeading = "O que será instalado",
                WhatYouAreInstallingBody = "• Leia e organize seus livros e documentos\n• Salve trechos, notas e seu progresso de leitura\n• Mantenha sua biblioteca privada no seu computador\n• Pronto para abrir após a instalação",
                PhaseNote = "Um espaço tranquilo para sua biblioteca pessoal. Novos formatos e recursos de estudo podem chegar nas próximas atualizações.",
                Close = "Fechar",
                Continue = "Continuar",
                ReadyToInstallHeading = "Pronto para instalar?",
                ReadyToInstallBody = "Escolha onde o Lumina Reader será instalado e quais atalhos deseja criar.",
                InstallationLocationHeading = "Local da instalação",
                Browse = "Procurar...",
                SelectInstallFolder = "Escolha onde instalar o Lumina Reader",
                StartMenuShortcut = "Criar um atalho no menu Iniciar",
                DesktopShortcut = "Criar um atalho na área de trabalho",
                ConfirmNote = "Escolha Instalar para continuar ou Fechar para sair sem fazer alterações.",
                Back = "Voltar",
                Install = "Instalar",
                InstallingHeading = "Instalando o Lumina Reader",
                InstallingBody = "Aguarde enquanto o Lumina Reader é instalado. Isso deve levar apenas alguns instantes.",
                PreparingInstallation = "Preparando a instalação...",
                InstallingButton = "Instalando...",
                InstallationCompleteHeading = "Instalação concluída",
                InstallationCompleteBody = "O Lumina Reader está pronto para uso. Os atalhos escolhidos foram criados.",
                InstalledToHeading = "Instalado em",
                InstallationCompleteNote = "Clique em Abrir para iniciar o Lumina Reader agora ou em Fechar para concluir.",
                Launch = "Abrir Lumina Reader",
                InstallingFileTemplate = "Instalando {0}...",
                RegisteringApplication = "Registrando o Lumina Reader no Windows...",
                CreatingShortcuts = "Criando atalhos...",
                InstallationCompleteStatus = "Instalação concluída.",
                InstallationError = "Não foi possível instalar o Lumina Reader.\n\n{0}",
                LaunchError = "Não foi possível abrir o Lumina Reader.\n\n{0}",
                PackageMissing = "O pacote do aplicativo não foi encontrado.",
                ExecutableMissing = "O executável do aplicativo não foi encontrado após a instalação.",
                UninstallerMissing = "O componente de desinstalação não foi encontrado.",
                UninstallRegistrationFailed = "Não foi possível registrar o Lumina Reader para desinstalação no Windows.",
                InvalidPackagePath = "O pacote do aplicativo contém um caminho inválido."
            };
        }
    }

    private readonly record struct InstallProgress(int Percent, string Status);
}

