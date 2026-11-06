using System.Globalization;

internal readonly record struct InstallerLanguageOption(string Locale, string Label);

internal static class InstallerLanguageCatalog
{
    public static IReadOnlyList<InstallerLanguageOption> All { get; } =
    [
        new("ar-SA", "Arabic (Saudi Arabia)"),
        new("eu-ES", "Basque (Basque)"),
        new("bg-BG", "Bulgarian (Bulgaria)"),
        new("ca-ES", "Catalan"),
        new("zh-HK", "Chinese (Traditional, Hong Kong SAR)"),
        new("zh-CN", "Chinese (Simplified, China)"),
        new("zh-TW", "Chinese (Traditional, Taiwan)"),
        new("hr-HR", "Croatian (Croatia)"),
        new("cs-CZ", "Czech (Czech Republic)"),
        new("da-DK", "Danish (Denmark)"),
        new("nl-NL", "Dutch (Netherlands)"),
        new("en-US", "English (United States)"),
        new("en-GB", "English (United Kingdom)"),
        new("et-EE", "Estonian (Estonia)"),
        new("fi-FI", "Finnish (Finland)"),
        new("fr-CA", "French (Canada)"),
        new("fr-FR", "French (France)"),
        new("gl-ES", "Galician"),
        new("de-DE", "German (Germany)"),
        new("el-GR", "Greek (Greece)"),
        new("he-IL", "Hebrew (Israel)"),
        new("hu-HU", "Hungarian (Hungary)"),
        new("id-ID", "Indonesian (Indonesia)"),
        new("it-IT", "Italian (Italy)"),
        new("ja-JP", "Japanese (Japan)"),
        new("ko-KR", "Korean (Korea)"),
        new("lv-LV", "Latvian (Latvia)"),
        new("lt-LT", "Lithuanian (Lithuania)"),
        new("nb-NO", "Norwegian, Bokmål (Norway)"),
        new("pl-PL", "Polish (Poland)"),
        new("pt-BR", "Portuguese (Brazil)"),
        new("pt-PT", "Portuguese (Portugal)"),
        new("ro-RO", "Romanian (Romania)"),
        new("ru-RU", "Russian (Russia)"),
        new("sr-Latn-CS", "Serbian (Latin, Serbia)"),
        new("sr-Latn-RS", "Serbian (Latin, Serbia)"),
        new("sk-SK", "Slovak (Slovakia)"),
        new("sl-SI", "Slovenian (Slovenia)"),
        new("es-MX", "Spanish (Mexico)"),
        new("es-ES", "Spanish (Spain)"),
        new("sv-SE", "Swedish (Sweden)"),
        new("th-TH", "Thai (Thailand)"),
        new("tr-TR", "Turkish (Türkiye)"),
        new("uk-UA", "Ukrainian (Ukraine)"),
        new("vi-VN", "Vietnamese"),
        new("af-ZA", "Afrikaans (South Africa)"),
        new("sq-AL", "Albanian (Albania)"),
        new("am-ET", "Amharic (Ethiopia)"),
        new("hy-AM", "Armenian (Armenia)"),
        new("as-IN", "Assamese (India)"),
        new("az-Latn-AZ", "Azerbaijani (Latin)"),
        new("be-BY", "Belarusian"),
        new("bn-IN", "Bangla (India)"),
        new("bs-Latn-BA", "Bosnian (Latin)"),
        new("chr-CHER-US", "Cherokee"),
        new("fil-PH", "Filipino"),
        new("ka-GE", "Georgian (Georgia)"),
        new("gu-IN", "Gujarati (India)"),
        new("hi-IN", "Hindi (India)"),
        new("is-IS", "Icelandic (Iceland)"),
        new("ga-IE", "Irish (Ireland)"),
        new("kn-IN", "Kannada (India)"),
        new("kk-KZ", "Kazakh (Kazakhstan)"),
        new("km-KH", "Khmer (Cambodia)"),
        new("kok-IN", "Konkani (India)"),
        new("lo-LA", "Lao (Laos)"),
        new("lb-LU", "Luxembourgish (Luxembourg)"),
        new("mk-MK", "Macedonian (North Macedonia)"),
        new("ms-MY", "Malay (Malaysia, Brunei, and Singapore)"),
        new("ml-IN", "Malayalam (India)"),
        new("mt-MT", "Maltese (Malta)"),
        new("mi-NZ", "Maori (New Zealand)"),
        new("mr-IN", "Marathi (India)"),
        new("ne-NP", "Nepali (Nepal)"),
        new("nn-NO", "Norwegian, Nynorsk (Norway)"),
        new("or-IN", "Odia (India)"),
        new("fa-IR", "Persian"),
        new("pa-IN", "Punjabi (India)"),
        new("quz-PE", "Quechua (Peru)"),
        new("gd-GB", "Scottish Gaelic"),
        new("sr-Cyrl-BA", "Serbian (Cyrillic, Bosnia and Herzegovina)"),
        new("sr-Cyrl-CS", "Serbian (Cyrillic, Serbia)"),
        new("sr-Cyrl-RS", "Serbian (Cyrillic, Serbia)"),
        new("ta-IN", "Tamil (India)"),
        new("tt-RU", "Tatar (Russia)"),
        new("te-IN", "Telugu (India)"),
        new("ur-PK", "Urdu"),
        new("ug-CN", "Uyghur"),
        new("uz-Latn-UZ", "Uzbek (Latin)"),
        new("ca-ES-valencia", "Valencian"),
        new("cy-GB", "Welsh (Great Britain)"),
    ];

    public static IReadOnlyList<InstallerLanguageOption> Visible { get; } =
        BuildVisibleOptions();

    public static string DetectSystemLocale()
    {
        var cultures = new[]
        {
            CultureInfo.CurrentUICulture,
            CultureInfo.InstalledUICulture,
            CultureInfo.CurrentCulture
        };

        foreach (var culture in cultures)
        {
            var exact = Find(culture.Name);
            if (exact is not null)
            {
                return exact.Value.Locale;
            }

            var language = culture.TwoLetterISOLanguageName;
            var languageMatch = All.FirstOrDefault(option =>
                option.Locale.StartsWith(language + "-", StringComparison.OrdinalIgnoreCase));
            if (!string.IsNullOrEmpty(languageMatch.Locale))
            {
                return languageMatch.Locale;
            }
        }

        return "en-US";
    }

    public static InstallerLanguageOption? Find(string locale)
    {
        return All.FirstOrDefault(option =>
            option.Locale.Equals(locale.Replace('_', '-'), StringComparison.OrdinalIgnoreCase));
    }

    public static InstallerLanguageOption? FindByIndex(int index)
    {
        return index >= 0 && index < Visible.Count ? Visible[index] : null;
    }

    public static int FindVisibleIndex(string locale)
    {
        var normalized = locale.Replace('_', '-');
        var selected = All.FirstOrDefault(option =>
            option.Locale.Equals(normalized, StringComparison.OrdinalIgnoreCase));
        if (string.IsNullOrEmpty(selected.Locale))
        {
            return 0;
        }

        var visibleIndex = Visible
            .Select((option, index) => (option, index))
            .FirstOrDefault(item => item.option.Label.Equals(
                selected.Label,
                StringComparison.OrdinalIgnoreCase));
        return visibleIndex.option.Equals(default(InstallerLanguageOption))
            ? 0
            : visibleIndex.index;
    }

    public static bool IsRightToLeft(string locale)
    {
        var language = locale
            .Replace('_', '-')
            .Split('-', StringSplitOptions.RemoveEmptyEntries)
            .FirstOrDefault()
            ?.ToLowerInvariant();
        return language is "ar" or "fa" or "he" or "ug" or "ur";
    }

    private static IReadOnlyList<InstallerLanguageOption> BuildVisibleOptions()
    {
        return All
            .GroupBy(option => option.Label, StringComparer.OrdinalIgnoreCase)
            .Select(group => group
                .OrderBy(option => IsCompatibilityAlias(option) ? 1 : 0)
                .First())
            .ToArray();
    }

    private static bool IsCompatibilityAlias(InstallerLanguageOption option)
    {
        return option.Locale.Equals("sr-Latn-CS", StringComparison.OrdinalIgnoreCase)
            || option.Locale.Equals("sr-Cyrl-CS", StringComparison.OrdinalIgnoreCase);
    }

}
