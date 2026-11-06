using Microsoft.Win32;
using System.Diagnostics;
using System.Globalization;

internal static class Program
{
    private const string UninstallRegistryPath =
        @"Software\Microsoft\Windows\CurrentVersion\Uninstall\LuminaReader";

    [STAThread]
    private static int Main(string[] args)
    {
        ApplicationConfiguration.Initialize();

        var strings = UninstallerStrings.ForSystemLanguage();
        var quiet = args.Any(argument => argument.Equals("--quiet", StringComparison.OrdinalIgnoreCase));
        var installRoot = Path.GetFullPath(AppContext.BaseDirectory.TrimEnd(Path.DirectorySeparatorChar));

        if (!quiet)
        {
            var confirmation = MessageBox.Show(
                strings.Confirmation,
                strings.Title,
                MessageBoxButtons.YesNo,
                MessageBoxIcon.Warning,
                MessageBoxDefaultButton.Button2);
            if (confirmation != DialogResult.Yes)
            {
                return 0;
            }
        }

        try
        {
            CloseRunningApplication(installRoot, strings);
            RemoveShortcuts();
            RemoveRegistryEntry();
            ScheduleInstallationDirectoryRemoval(installRoot);

            if (!quiet)
            {
                MessageBox.Show(
                    strings.Completed,
                    strings.Title,
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
            }

            return 0;
        }
        catch (Exception exception)
        {
            if (!quiet)
            {
                MessageBox.Show(
                    string.Format(strings.Error, exception.Message),
                    strings.Title,
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }

            return 1;
        }
    }

    private static void CloseRunningApplication(string installRoot, UninstallerStrings strings)
    {
        var expectedExecutable = Path.Combine(installRoot, "lumina_reader.exe");
        foreach (var process in Process.GetProcessesByName("lumina_reader"))
        {
            using (process)
            {
                try
                {
                    var executable = process.MainModule?.FileName;
                    if (!string.Equals(executable, expectedExecutable, StringComparison.OrdinalIgnoreCase))
                    {
                        continue;
                    }

                    process.CloseMainWindow();
                    if (!process.WaitForExit(5000))
                    {
                        throw new InvalidOperationException(strings.ApplicationRunning);
                    }
                }
                catch (System.ComponentModel.Win32Exception)
                {
                    // The process may already have exited between enumeration and inspection.
                }
            }
        }
    }

    private static void RemoveShortcuts()
    {
        var startMenuDirectory = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.StartMenu),
            "Programs",
            "Lumina Reader");
        var startMenuShortcut = Path.Combine(startMenuDirectory, "Lumina Reader.lnk");
        var desktopShortcut = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory),
            "Lumina Reader.lnk");

        DeleteFileIfPresent(startMenuShortcut);
        DeleteFileIfPresent(desktopShortcut);

        if (Directory.Exists(startMenuDirectory) && !Directory.EnumerateFileSystemEntries(startMenuDirectory).Any())
        {
            Directory.Delete(startMenuDirectory);
        }
    }

    private static void DeleteFileIfPresent(string path)
    {
        if (File.Exists(path))
        {
            File.Delete(path);
        }
    }

    private static void RemoveRegistryEntry()
    {
        Registry.CurrentUser.DeleteSubKeyTree(UninstallRegistryPath, throwOnMissingSubKey: false);
    }

    private static void ScheduleInstallationDirectoryRemoval(string installRoot)
    {
        var commandShell = Environment.GetEnvironmentVariable("ComSpec") ?? "cmd.exe";
        var command = $"/C timeout /T 1 /NOBREAK > nul & rmdir /S /Q \"{installRoot}\"";
        var process = Process.Start(new ProcessStartInfo
        {
            FileName = commandShell,
            Arguments = command,
            CreateNoWindow = true,
            UseShellExecute = false,
            WindowStyle = ProcessWindowStyle.Hidden
        });

        if (process is null)
        {
            throw new InvalidOperationException("The cleanup process could not be started.");
        }
    }

    private sealed class UninstallerStrings
    {
        public string Title { get; init; } = string.Empty;
        public string Confirmation { get; init; } = string.Empty;
        public string Completed { get; init; } = string.Empty;
        public string Error { get; init; } = string.Empty;
        public string ApplicationRunning { get; init; } = string.Empty;

        public static UninstallerStrings ForSystemLanguage()
        {
            var cultures = new[]
            {
                CultureInfo.CurrentUICulture,
                CultureInfo.InstalledUICulture,
                CultureInfo.CurrentCulture
            };
            var isPortuguese = cultures.Any(culture =>
                culture.Name.StartsWith("pt", StringComparison.OrdinalIgnoreCase)
                || culture.TwoLetterISOLanguageName.Equals("pt", StringComparison.OrdinalIgnoreCase));
            return isPortuguese
                ? Portuguese()
                : English();
        }

        private static UninstallerStrings English()
        {
            return new UninstallerStrings
            {
                Title = "Lumina Reader Uninstall",
                Confirmation = "Do you want to remove Lumina Reader from this computer?",
                Completed = "Lumina Reader was removed successfully.",
                Error = "Lumina Reader could not be removed.\n\n{0}",
                ApplicationRunning = "Close Lumina Reader before uninstalling it."
            };
        }

        private static UninstallerStrings Portuguese()
        {
            return new UninstallerStrings
            {
                Title = "Desinstalação do Lumina Reader",
                Confirmation = "Deseja remover o Lumina Reader deste computador?",
                Completed = "O Lumina Reader foi removido com sucesso.",
                Error = "Não foi possível remover o Lumina Reader.\n\n{0}",
                ApplicationRunning = "Feche o Lumina Reader antes de desinstalá-lo."
            };
        }
    }
}
