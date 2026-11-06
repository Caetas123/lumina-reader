using System.IO.Compression;
using System.Reflection;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;

internal static class InstallerLocalizationCatalog
{
    private static readonly Regex TranslatedTokenMarker = new(
        @"(?:_LUMINA_TOKEN_|__[^_\s]+_(?:TOKEN|token|टोकन|टोकेन|ТОКЕН)_)(\d+)__",
        RegexOptions.Compiled | RegexOptions.CultureInvariant);

    private static readonly Lazy<JsonDocument?> Catalog = new(LoadCatalog);

    public static IReadOnlyDictionary<string, string>? ForLocale(string locale)
    {
        var document = Catalog.Value;
        if (document is null)
        {
            return null;
        }

        var key = CatalogKey(locale);
        if (!document.RootElement.TryGetProperty("catalogs", out var catalogs)
            || !catalogs.TryGetProperty(key, out var catalog)
            || !catalog.TryGetProperty("installer", out var installer))
        {
            return null;
        }

        var translations = new Dictionary<string, string>(StringComparer.Ordinal);
        foreach (var property in installer.EnumerateObject())
        {
            translations[property.Name] = RestoreRuntimeMarkers(
                property.Name,
                property.Value.GetString() ?? string.Empty);
        }

        return translations;
    }

    private static string RestoreRuntimeMarkers(string key, string value)
    {
        var restored = value
            .Replace("_LUMINA_NEWLINE___", "_LUMINA_NEWLINE_", StringComparison.Ordinal)
            .Replace("_LUMINA_NEWLINE_", "\n", StringComparison.Ordinal)
            .Replace("__ LUMINA_NEWLINE__", "\n", StringComparison.Ordinal)
            .Replace("__LUMINA_NEWLINE__", "\n", StringComparison.Ordinal)
            .Replace("| LUMINA_NEWLINE__", "\n", StringComparison.Ordinal)
            .Replace("__LUMINA_MUSUQ CHIQA__", "\n", StringComparison.Ordinal)
            .Replace("__LUMINA_NEVLINE__", "\n", StringComparison.Ordinal)
            .Replace("__ЛУМИНА_НЕВЛИНЕ__", "\n", StringComparison.Ordinal);
        restored = TranslatedTokenMarker.Replace(restored, match =>
        {
            return int.TryParse(match.Groups[1].Value, out var zeroBasedIndex)
                ? $"{{{zeroBasedIndex}}}"
                : match.Value;
        });

        if (key.Equals("BrandVersionTemplate", StringComparison.Ordinal)
            && !restored.Contains("{0}", StringComparison.Ordinal))
        {
            restored += " {0}";
        }

        if (key is "InstallationError" or "LaunchError")
        {
            var tokenIndex = restored.IndexOf("{0}", StringComparison.Ordinal);
            if (tokenIndex < 0)
            {
                restored = restored.TrimEnd() + "\n\n{0}";
            }
            else
            {
                var message = restored[..tokenIndex].TrimEnd('\r', '\n');
                var details = restored[(tokenIndex + 3)..];
                restored = message + "\n\n{0}" + details;
            }
        }

        return restored;
    }

    private static string CatalogKey(string locale)
    {
        return locale.Replace('_', '-').ToLowerInvariant();
    }

    private static JsonDocument? LoadCatalog()
    {
        try
        {
            using var resource = Assembly.GetExecutingAssembly()
                .GetManifestResourceStream("LuminaLocalizationCatalog.txt");
            if (resource is null)
            {
                return null;
            }

            using var reader = new StreamReader(resource, Encoding.UTF8);
            var encoded = reader.ReadToEnd();
            var compressed = Convert.FromBase64String(encoded);
            if (compressed.Length <= 4)
            {
                return null;
            }

            using var compressedStream = new MemoryStream(
                compressed,
                4,
                compressed.Length - 4,
                writable: false);
            using var zlib = new ZLibStream(compressedStream, CompressionMode.Decompress);
            using var jsonReader = new StreamReader(zlib, Encoding.UTF8);
            return JsonDocument.Parse(jsonReader.ReadToEnd());
        }
        catch
        {
            return null;
        }
    }
}
