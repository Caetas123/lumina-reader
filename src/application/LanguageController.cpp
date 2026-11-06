#include "application/LanguageController.h"

#include "application/SettingsController.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qfile.h>
#include <QtCore/qhash.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qlocale.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qset.h>
#include <QtGui/qguiapplication.h>
#include <QtQml/qqmlengine.h>

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <limits>

namespace {

struct LanguageEntry {
    const char *locale;
    const char *label;
};

// Windows 11 language packs and the Windows 11 LIP catalog, including the
// current regional and script variants.
constexpr LanguageEntry kLanguages[] = {
    {"ar-SA", "Arabic (Saudi Arabia)"},
    {"eu-ES", "Basque (Basque)"},
    {"bg-BG", "Bulgarian (Bulgaria)"},
    {"ca-ES", "Catalan"},
    {"zh-HK", "Chinese (Traditional, Hong Kong SAR)"},
    {"zh-CN", "Chinese (Simplified, China)"},
    {"zh-TW", "Chinese (Traditional, Taiwan)"},
    {"hr-HR", "Croatian (Croatia)"},
    {"cs-CZ", "Czech (Czech Republic)"},
    {"da-DK", "Danish (Denmark)"},
    {"nl-NL", "Dutch (Netherlands)"},
    {"en-US", "English (United States)"},
    {"en-GB", "English (United Kingdom)"},
    {"et-EE", "Estonian (Estonia)"},
    {"fi-FI", "Finnish (Finland)"},
    {"fr-CA", "French (Canada)"},
    {"fr-FR", "French (France)"},
    {"gl-ES", "Galician"},
    {"de-DE", "German (Germany)"},
    {"el-GR", "Greek (Greece)"},
    {"he-IL", "Hebrew (Israel)"},
    {"hu-HU", "Hungarian (Hungary)"},
    {"id-ID", "Indonesian (Indonesia)"},
    {"it-IT", "Italian (Italy)"},
    {"ja-JP", "Japanese (Japan)"},
    {"ko-KR", "Korean (Korea)"},
    {"lv-LV", "Latvian (Latvia)"},
    {"lt-LT", "Lithuanian (Lithuania)"},
    {"nb-NO", "Norwegian, Bokmål (Norway)"},
    {"pl-PL", "Polish (Poland)"},
    {"pt-BR", "Portuguese (Brazil)"},
    {"pt-PT", "Portuguese (Portugal)"},
    {"ro-RO", "Romanian (Romania)"},
    {"ru-RU", "Russian (Russia)"},
    {"sr-Latn-CS", "Serbian (Latin, Serbia)"},
    {"sr-Latn-RS", "Serbian (Latin, Serbia)"},
    {"sk-SK", "Slovak (Slovakia)"},
    {"sl-SI", "Slovenian (Slovenia)"},
    {"es-MX", "Spanish (Mexico)"},
    {"es-ES", "Spanish (Spain)"},
    {"sv-SE", "Swedish (Sweden)"},
    {"th-TH", "Thai (Thailand)"},
    {"tr-TR", "Turkish (Türkiye)"},
    {"uk-UA", "Ukrainian (Ukraine)"},
    {"vi-VN", "Vietnamese"},
    {"af-ZA", "Afrikaans (South Africa)"},
    {"sq-AL", "Albanian (Albania)"},
    {"am-ET", "Amharic (Ethiopia)"},
    {"hy-AM", "Armenian (Armenia)"},
    {"as-IN", "Assamese (India)"},
    {"az-Latn-AZ", "Azerbaijani (Latin)"},
    {"be-BY", "Belarusian"},
    {"bn-IN", "Bangla (India)"},
    {"bs-Latn-BA", "Bosnian (Latin)"},
    {"chr-Cher-US", "Cherokee"},
    {"fil-PH", "Filipino"},
    {"ka-GE", "Georgian (Georgia)"},
    {"gu-IN", "Gujarati (India)"},
    {"hi-IN", "Hindi (India)"},
    {"is-IS", "Icelandic (Iceland)"},
    {"ga-IE", "Irish (Ireland)"},
    {"kn-IN", "Kannada (India)"},
    {"kk-KZ", "Kazakh (Kazakhstan)"},
    {"km-KH", "Khmer (Cambodia)"},
    {"kok-IN", "Konkani (India)"},
    {"lo-LA", "Lao (Laos)"},
    {"lb-LU", "Luxembourgish (Luxembourg)"},
    {"mk-MK", "Macedonian (North Macedonia)"},
    {"ms-MY", "Malay (Malaysia, Brunei, and Singapore)"},
    {"ml-IN", "Malayalam (India)"},
    {"mt-MT", "Maltese (Malta)"},
    {"mi-NZ", "Maori (New Zealand)"},
    {"mr-IN", "Marathi (India)"},
    {"ne-NP", "Nepali (Nepal)"},
    {"nn-NO", "Norwegian, Nynorsk (Norway)"},
    {"or-IN", "Odia (India)"},
    {"fa-IR", "Persian"},
    {"pa-IN", "Punjabi (India)"},
    {"quz-PE", "Quechua (Peru)"},
    {"gd-GB", "Scottish Gaelic"},
    {"sr-Cyrl-BA", "Serbian (Cyrillic, Bosnia and Herzegovina)"},
    {"sr-Cyrl-CS", "Serbian (Cyrillic, Serbia)"},
    {"sr-Cyrl-RS", "Serbian (Cyrillic, Serbia)"},
    {"ta-IN", "Tamil (India)"},
    {"tt-RU", "Tatar (Russia)"},
    {"te-IN", "Telugu (India)"},
    {"ur-PK", "Urdu"},
    {"ug-CN", "Uyghur"},
    {"uz-Latn-UZ", "Uzbek (Latin)"},
    {"ca-ES-valencia", "Valencian"},
    {"cy-GB", "Welsh (Great Britain)"},
};

QString canonicalLocale(const QString &value)
{
    QStringList parts = value.trimmed().replace(QLatin1Char('_'), QLatin1Char('-')).split(
        QLatin1Char('-'), Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return {};
    }

    parts[0] = parts[0].toLower();
    for (int index = 1; index < parts.size(); ++index) {
        if (parts[index].size() == 4) {
            parts[index] = parts[index].left(1).toUpper() + parts[index].mid(1).toLower();
        } else if (parts[index].size() == 2 || parts[index].size() == 3) {
            parts[index] = parts[index].toUpper();
        }
    }
    return parts.join(QLatin1Char('-'));
}

QString languagePart(const QString &locale)
{
    return locale.section(QLatin1Char('-'), 0, 0).toLower();
}

bool isRightToLeftLanguage(const QString &locale)
{
    const QString language = languagePart(locale);
    return language == QStringLiteral("ar")
        || language == QStringLiteral("fa")
        || language == QStringLiteral("he")
        || language == QStringLiteral("ug")
        || language == QStringLiteral("ur");
}

bool isCompatibilityAlias(const LanguageEntry &entry)
{
    return qstrcmp(entry.locale, "sr-Latn-CS") == 0
        || qstrcmp(entry.locale, "sr-Cyrl-CS") == 0;
}

QString displayLocaleForValue(const QString &locale)
{
    const QString canonical = canonicalLocale(locale);
    if (canonical == QStringLiteral("sr-Latn-CS")) {
        return QStringLiteral("sr-Latn-RS");
    }
    if (canonical == QStringLiteral("sr-Cyrl-CS")) {
        return QStringLiteral("sr-Cyrl-RS");
    }
    return canonical;
}

QString labelFor(const QString &locale)
{
    const QString canonical = canonicalLocale(locale);
    for (const LanguageEntry &entry : kLanguages) {
        if (canonical == QLatin1StringView(entry.locale)) {
            return QString::fromUtf8(entry.label);
        }
    }
    return canonical;
}

const QHash<QString, QString> &portugueseTranslations()
{
    // The source UI remains English for every string without a catalog entry.
    // Keeping this dictionary in the binary makes the selected language fully
    // offline and lets QML retranslate without shipping a writable file.
    static const QHash<QString, QString> translations = {
        {QStringLiteral("Everywhere"), QStringLiteral("Em todo lugar")},
        {QStringLiteral("This format"), QStringLiteral("Este formato")},
        {QStringLiteral("This content type"), QStringLiteral("Este tipo de conteúdo")},
        {QStringLiteral("This document"), QStringLiteral("Este documento")},
        {QStringLiteral("Book"), QStringLiteral("Livro")},
        {QStringLiteral("Manga"), QStringLiteral("Mangá")},
        {QStringLiteral("Manhwa"), QStringLiteral("Manhwa")},
        {QStringLiteral("Webtoon"), QStringLiteral("Webtoon")},
        {QStringLiteral("Newspaper"), QStringLiteral("Jornal")},
        {QStringLiteral("Magazine"), QStringLiteral("Revista")},
        {QStringLiteral("Article"), QStringLiteral("Artigo")},
        {QStringLiteral("Academic paper"), QStringLiteral("Artigo acadêmico")},
        {QStringLiteral("Handout"), QStringLiteral("Apostila")},
        {QStringLiteral("Comic"), QStringLiteral("Quadrinhos")},
        {QStringLiteral("Document"), QStringLiteral("Documento")},
        {QStringLiteral("Image"), QStringLiteral("Imagem")},
        {QStringLiteral("Text"), QStringLiteral("Texto")},
        {QStringLiteral("Other"), QStringLiteral("Outro")},
        {QStringLiteral("Note"), QStringLiteral("Nota")},
        {QStringLiteral("Image region"), QStringLiteral("Região da imagem")},
        {QStringLiteral("English"), QStringLiteral("Inglês")},
        {QStringLiteral("Portuguese"), QStringLiteral("Português")},
        {QStringLiteral("Spanish"), QStringLiteral("Espanhol")},
        {QStringLiteral("French"), QStringLiteral("Francês")},
        {QStringLiteral("German"), QStringLiteral("Alemão")},
        {QStringLiteral("Japanese"), QStringLiteral("Japonês")},
        {QStringLiteral("Korean"), QStringLiteral("Coreano")},
        {QStringLiteral("Chinese"), QStringLiteral("Chinês")},
        {QStringLiteral("Recently opened"), QStringLiteral("Abertos recentemente")},
        {QStringLiteral("Date added"), QStringLiteral("Data de adição")},
        {QStringLiteral("Title"), QStringLiteral("Título")},
        {QStringLiteral("Author"), QStringLiteral("Autor")},
        {QStringLiteral("Progress"), QStringLiteral("Progresso")},
        {QStringLiteral("Rating"), QStringLiteral("Avaliação")},
        {QStringLiteral("Added %1 document(s) to your library."), QStringLiteral("%1 documento(s) adicionado(s) à sua biblioteca.")},
        {QStringLiteral("The document could not be opened."), QStringLiteral("Não foi possível abrir o documento.")},
        {QStringLiteral("Reading"), QStringLiteral("Lendo")},
        {QStringLiteral("Finished"), QStringLiteral("Concluídos")},
        {QStringLiteral("Abandoned"), QStringLiteral("Abandonados")},
        {QStringLiteral("To read"), QStringLiteral("Para ler")},
        {QStringLiteral("%1 min read"), QStringLiteral("%1 min de leitura")},
        {QStringLiteral("%1 h read"), QStringLiteral("%1 h de leitura")},
        {QStringLiteral("%1 h %2 min read"), QStringLiteral("%1 h %2 min de leitura")},
        {QStringLiteral("All items"), QStringLiteral("Todos os itens")},
        {QStringLiteral("Favorites"), QStringLiteral("Favoritos")},
        {QStringLiteral("Collection: %1"), QStringLiteral("Coleção: %1")},
        {QStringLiteral("Tag: %1"), QStringLiteral("Etiqueta: %1")},
        {QStringLiteral("Genre: %1"), QStringLiteral("Gênero: %1")},
        {QStringLiteral("Language: %1"), QStringLiteral("Idioma: %1")},
        {QStringLiteral("Format: %1"), QStringLiteral("Formato: %1")},
        {QStringLiteral("Add documents to Lumina Reader"), QStringLiteral("Adicionar documentos ao Lumina Reader")},
        {QStringLiteral("Supported documents (%1)"), QStringLiteral("Documentos compatíveis (%1)")},
        {QStringLiteral("All files (*)"), QStringLiteral("Todos os arquivos (*)")},
        {QStringLiteral("Locate the original document"), QStringLiteral("Localizar o documento original")},
        {QStringLiteral("The document location was restored."), QStringLiteral("O local do documento foi restaurado.")},
        {QStringLiteral("That file does not match the saved document."), QStringLiteral("Esse arquivo não corresponde ao documento salvo.")},
        {QStringLiteral("Add a folder to Lumina Reader"), QStringLiteral("Adicionar uma pasta ao Lumina Reader")},
        {QStringLiteral("The folder was added to your library."), QStringLiteral("A pasta foi adicionada à sua biblioteca.")},
        {QStringLiteral("No supported documents were found in that folder."), QStringLiteral("Nenhum documento compatível foi encontrado nessa pasta.")},
        {QStringLiteral("Watch a folder for new documents"), QStringLiteral("Monitorar uma pasta por novos documentos")},
        {QStringLiteral("Lumina Reader will watch this folder for new documents."), QStringLiteral("O Lumina Reader vai monitorar esta pasta por novos documentos.")},
        {QStringLiteral("The selected folder could not be watched."), QStringLiteral("Não foi possível monitorar a pasta selecionada.")},
        {QStringLiteral("Edit document details"), QStringLiteral("Editar detalhes do documento")},
        {QStringLiteral("These details are stored in your local library and do not change the original file."), QStringLiteral("Esses detalhes ficam na sua biblioteca local e não alteram o arquivo original.")},
        {QStringLiteral("Original file not found: %1"), QStringLiteral("Arquivo original não encontrado: %1")},
        {QStringLiteral("Locate file"), QStringLiteral("Localizar arquivo")},
        {QStringLiteral("Language"), QStringLiteral("Idioma")},
        {QStringLiteral("Genre"), QStringLiteral("Gênero")},
        {QStringLiteral("Tags, separated by commas"), QStringLiteral("Etiquetas, separadas por vírgulas")},
        {QStringLiteral("Choose a collection"), QStringLiteral("Escolha uma coleção")},
        {QStringLiteral("Add"), QStringLiteral("Adicionar")},
        {QStringLiteral("Document added to the collection."), QStringLiteral("Documento adicionado à coleção.")},
        {QStringLiteral("The document could not be added to the collection."), QStringLiteral("Não foi possível adicionar o documento à coleção.")},
        {QStringLiteral("Remove"), QStringLiteral("Remover")},
        {QStringLiteral("Document removed from the collection."), QStringLiteral("Documento removido da coleção.")},
        {QStringLiteral("The document is not in that collection."), QStringLiteral("O documento não está nessa coleção.")},
        {QStringLiteral("No cover image selected"), QStringLiteral("Nenhuma capa selecionada")},
        {QStringLiteral("Choose cover"), QStringLiteral("Escolher capa")},
        {QStringLiteral("Clear"), QStringLiteral("Limpar")},
        {QStringLiteral("The cover image was cleared."), QStringLiteral("A capa foi removida.")},
        {QStringLiteral("Personal description"), QStringLiteral("Descrição pessoal")},
        {QStringLiteral("Remove from library"), QStringLiteral("Remover da biblioteca")},
        {QStringLiteral("Document details saved."), QStringLiteral("Detalhes do documento salvos.")},
        {QStringLiteral("The document details could not be saved."), QStringLiteral("Não foi possível salvar os detalhes do documento.")},
        {QStringLiteral("Remove document"), QStringLiteral("Remover documento")},
        {QStringLiteral("Also delete the original file from the device"), QStringLiteral("Excluir também o arquivo original do dispositivo")},
        {QStringLiteral("The document was removed from your library."), QStringLiteral("O documento foi removido da sua biblioteca.")},
        {QStringLiteral("The document could not be removed."), QStringLiteral("Não foi possível remover o documento.")},
        {QStringLiteral("Choose a cover image"), QStringLiteral("Escolher uma imagem de capa")},
        {QStringLiteral("The cover image was saved."), QStringLiteral("A capa foi salva.")},
        {QStringLiteral("The selected cover image could not be used."), QStringLiteral("Não foi possível usar a capa selecionada.")},
        {QStringLiteral("Create collection"), QStringLiteral("Criar coleção")},
        {QStringLiteral("Collection name"), QStringLiteral("Nome da coleção")},
        {QStringLiteral("Description (optional)"), QStringLiteral("Descrição (opcional)")},
        {QStringLiteral("Collection created."), QStringLiteral("Coleção criada.")},
        {QStringLiteral("A collection with that name could not be created."), QStringLiteral("Não foi possível criar uma coleção com esse nome.")},
        {QStringLiteral("Create reading profile"), QStringLiteral("Criar perfil de leitura")},
        {QStringLiteral("Save a separate reading style for a study plan, a book format, or another reader."), QStringLiteral("Salve um estilo de leitura separado para um plano de estudo, formato de livro ou outro leitor.")},
        {QStringLiteral("Profile name"), QStringLiteral("Nome do perfil")},
        {QStringLiteral("Reading profile created."), QStringLiteral("Perfil de leitura criado.")},
        {QStringLiteral("Enter a unique profile name."), QStringLiteral("Digite um nome de perfil exclusivo.")},
        {QStringLiteral("Search history"), QStringLiteral("Histórico de pesquisa")},
        {QStringLiteral("Knowledge center"), QStringLiteral("Central de conhecimento")},
        {QStringLiteral("Your library"), QStringLiteral("Sua biblioteca")},
        {QStringLiteral("Recent searches in %1"), QStringLiteral("Pesquisas recentes em %1")},
        {QStringLiteral("Clear history"), QStringLiteral("Limpar histórico")},
        {QStringLiteral("Search history cleared."), QStringLiteral("Histórico de pesquisa limpo.")},
        {QStringLiteral("No saved searches yet"), QStringLiteral("Nenhuma pesquisa salva ainda")},
        {QStringLiteral("Searches you run here will appear in this list."), QStringLiteral("As pesquisas feitas aqui aparecerão nesta lista.")},
        {QStringLiteral("Use"), QStringLiteral("Usar")},
        {QStringLiteral("Close"), QStringLiteral("Fechar")},
        {QStringLiteral("Dictionary"), QStringLiteral("Dicionário")},
        {QStringLiteral("Translation"), QStringLiteral("Tradução")},
        {QStringLiteral("History"), QStringLiteral("Histórico")},
        {QStringLiteral("No saved lookups yet."), QStringLiteral("Nenhuma consulta salva ainda.")},
        {QStringLiteral("Auto"), QStringLiteral("Automático")},
        {QStringLiteral("Open"), QStringLiteral("Abrir")},
        {QStringLiteral("Remove from favorites"), QStringLiteral("Remover dos favoritos")},
        {QStringLiteral("Save to favorites"), QStringLiteral("Salvar nos favoritos")},
        {QStringLiteral("Copy result"), QStringLiteral("Copiar resultado")},
        {QStringLiteral("Result copied."), QStringLiteral("Resultado copiado.")},
        {QStringLiteral("Pages"), QStringLiteral("Páginas")},
        {QStringLiteral("Contents"), QStringLiteral("Conteúdo")},
        {QStringLiteral("Bookmarks"), QStringLiteral("Marcadores")},
        {QStringLiteral("Go"), QStringLiteral("Ir")},
        {QStringLiteral("Delete"), QStringLiteral("Excluir")},
        {QStringLiteral("Bookmark removed."), QStringLiteral("Marcador removido.")},
        {QStringLiteral("Export document text"), QStringLiteral("Exportar texto do documento")},
        {QStringLiteral("Plain text (*.txt)"), QStringLiteral("Texto simples (*.txt)")},
        {QStringLiteral("Document text exported successfully."), QStringLiteral("Texto do documento exportado com sucesso.")},
        {QStringLiteral("Export document as PDF"), QStringLiteral("Exportar documento como PDF")},
        {QStringLiteral("PDF document (*.pdf)"), QStringLiteral("Documento PDF (*.pdf)")},
        {QStringLiteral("PDF exported successfully."), QStringLiteral("PDF exportado com sucesso.")},
        {QStringLiteral("Export knowledge"), QStringLiteral("Exportar conhecimento")},
        {QStringLiteral("Knowledge exported successfully."), QStringLiteral("Conhecimento exportado com sucesso.")},
        {QStringLiteral("Export library"), QStringLiteral("Exportar biblioteca")},
        {QStringLiteral("Library exported successfully."), QStringLiteral("Biblioteca exportada com sucesso.")},
        {QStringLiteral("Restore a Lumina library export"), QStringLiteral("Restaurar uma exportação da biblioteca Lumina")},
        {QStringLiteral("Library metadata restored successfully."), QStringLiteral("Metadados da biblioteca restaurados com sucesso.")},
        {QStringLiteral("Export reading preferences"), QStringLiteral("Exportar preferências de leitura")},
        {QStringLiteral("Reading preferences exported successfully."), QStringLiteral("Preferências de leitura exportadas com sucesso.")},
        {QStringLiteral("Restore reading preferences"), QStringLiteral("Restaurar preferências de leitura")},
        {QStringLiteral("Reading preferences restored successfully."), QStringLiteral("Preferências de leitura restauradas com sucesso.")},
        {QStringLiteral("Create database backup"), QStringLiteral("Criar backup do banco de dados")},
        {QStringLiteral("Restore a Lumina database backup"), QStringLiteral("Restaurar um backup do banco de dados Lumina")},
        {QStringLiteral("Replace the local library?"), QStringLiteral("Substituir a biblioteca local?")},
        {QStringLiteral("Database restored successfully. The library was reloaded."), QStringLiteral("Banco de dados restaurado com sucesso. A biblioteca foi recarregada.")},
        {QStringLiteral("Lumina Reader"), QStringLiteral("Lumina Reader")},
        {QStringLiteral("Library"), QStringLiteral("Biblioteca")},
        {QStringLiteral("My library"), QStringLiteral("Minha biblioteca")},
        {QStringLiteral("Workspace"), QStringLiteral("Espaço de trabalho")},
        {QStringLiteral("Settings"), QStringLiteral("Configurações")},
        {QStringLiteral("Private by design"), QStringLiteral("Privacidade desde o início")},
        {QStringLiteral("Your library stays on this device."), QStringLiteral("Sua biblioteca fica neste dispositivo.")},
        {QStringLiteral("Welcome back"), QStringLiteral("Bem-vindo de volta")},
        {QStringLiteral("Add documents"), QStringLiteral("Adicionar documentos")},
        {QStringLiteral("Add folder"), QStringLiteral("Adicionar pasta")},
        {QStringLiteral("Watch folder"), QStringLiteral("Monitorar pasta")},
        {QStringLiteral("Grid view"), QStringLiteral("Visualização em grade")},
        {QStringLiteral("List view"), QStringLiteral("Visualização em lista")},
        {QStringLiteral("Search your library"), QStringLiteral("Pesquisar na sua biblioteca")},
        {QStringLiteral("%1 item(s)"), QStringLiteral("%1 item(ns)")},
        {QStringLiteral("No documents match your search."), QStringLiteral("Nenhum documento corresponde à sua pesquisa.")},
        {QStringLiteral("Library is empty"), QStringLiteral("A biblioteca está vazia")},
        {QStringLiteral("Try another search or add a document."), QStringLiteral("Tente outra pesquisa ou adicione um documento.")},
        {QStringLiteral("Add documents, a folder, or watch a folder to build your reading library."), QStringLiteral("Adicione documentos, uma pasta ou monitore uma pasta para criar sua biblioteca de leitura.")},
        {QStringLiteral("All documents"), QStringLiteral("Todos os documentos")},
        {QStringLiteral("Find the ideas, passages, and notes you saved across your library."), QStringLiteral("Encontre as ideias, trechos e notas que você salvou na sua biblioteca.")},
        {QStringLiteral("Undo"), QStringLiteral("Desfazer")},
        {QStringLiteral("Redo"), QStringLiteral("Refazer")},
        {QStringLiteral("Search highlights and notes"), QStringLiteral("Pesquisar destaques e notas")},
        {QStringLiteral("All formats"), QStringLiteral("Todos os formatos")},
        {QStringLiteral("All colors"), QStringLiteral("Todas as cores")},
        {QStringLiteral("Filter by author"), QStringLiteral("Filtrar por autor")},
        {QStringLiteral("Relevance"), QStringLiteral("Relevância")},
        {QStringLiteral("Newest"), QStringLiteral("Mais recentes")},
        {QStringLiteral("Oldest"), QStringLiteral("Mais antigos")},
        {QStringLiteral("Edit"), QStringLiteral("Editar")},
        {QStringLiteral("Your saved knowledge will appear here."), QStringLiteral("Seu conhecimento salvo aparecerá aqui.")},
        {QStringLiteral("No saved passages match this search."), QStringLiteral("Nenhum trecho salvo corresponde a esta pesquisa.")},
        {QStringLiteral("Back to library"), QStringLiteral("Voltar para a biblioteca")},
        {QStringLiteral("Find in document"), QStringLiteral("Localizar no documento")},
        {QStringLiteral("Export text"), QStringLiteral("Exportar texto")},
        {QStringLiteral("Export PDF"), QStringLiteral("Exportar PDF")},
        {QStringLiteral("Print"), QStringLiteral("Imprimir")},
        {QStringLiteral("Previous"), QStringLiteral("Anterior")},
        {QStringLiteral("Next"), QStringLiteral("Próximo")},
        {QStringLiteral("Exit focus"), QStringLiteral("Sair do foco")},
        {QStringLiteral("Focus"), QStringLiteral("Foco")},
        {QStringLiteral("Windowed"), QStringLiteral("Janela")},
        {QStringLiteral("Full screen"), QStringLiteral("Tela cheia")},
        {QStringLiteral("Rotate"), QStringLiteral("Girar")},
        {QStringLiteral("Open page thumbnails"), QStringLiteral("Abrir miniaturas das páginas")},
        {QStringLiteral("Open document contents"), QStringLiteral("Abrir conteúdo do documento")},
        {QStringLiteral("Zoom out (Ctrl+-)"), QStringLiteral("Afastar (Ctrl+-)")},
        {QStringLiteral("Zoom in (Ctrl++)"), QStringLiteral("Aproximar (Ctrl++)")},
        {QStringLiteral("Fit page (Ctrl+0)"), QStringLiteral("Ajustar à página (Ctrl+0)")},
        {QStringLiteral("Single page"), QStringLiteral("Uma página")},
        {QStringLiteral("Two pages"), QStringLiteral("Duas páginas")},
        {QStringLiteral("Show page"), QStringLiteral("Mostrar página")},
        {QStringLiteral("Show text"), QStringLiteral("Mostrar texto")},
        {QStringLiteral("Recognizing…"), QStringLiteral("Reconhecendo…")},
        {QStringLiteral("Read image"), QStringLiteral("Ler imagem")},
        {QStringLiteral("OCR language"), QStringLiteral("Idioma do OCR")},
        {QStringLiteral("Read all pages"), QStringLiteral("Ler todas as páginas")},
        {QStringLiteral("Document OCR progress"), QStringLiteral("Progresso do OCR do documento")},
        {QStringLiteral("Cancel OCR"), QStringLiteral("Cancelar OCR")},
        {QStringLiteral("Bookmark"), QStringLiteral("Marcador")},
        {QStringLiteral("Bookmark saved."), QStringLiteral("Marcador salvo.")},
        {QStringLiteral("Stop"), QStringLiteral("Parar")},
        {QStringLiteral("Read aloud"), QStringLiteral("Ler em voz alta")},
        {QStringLiteral("Resume"), QStringLiteral("Continuar")},
        {QStringLiteral("Pause"), QStringLiteral("Pausar")},
        {QStringLiteral("Choose a document from your library to start reading."), QStringLiteral("Escolha um documento da sua biblioteca para começar a ler.")},
        {QStringLiteral("Passage saved to the Knowledge center."), QStringLiteral("Trecho salvo na Central de conhecimento.")},
        {QStringLiteral("Comment"), QStringLiteral("Comentário")},
        {QStringLiteral("Copy"), QStringLiteral("Copiar")},
        {QStringLiteral("Google"), QStringLiteral("Google")},
        {QStringLiteral("Translation target language"), QStringLiteral("Idioma de destino da tradução")},
        {QStringLiteral("Translate"), QStringLiteral("Traduzir")},
        {QStringLiteral("Appearance"), QStringLiteral("Aparência")},
        {QStringLiteral("Files & library"), QStringLiteral("Arquivos e biblioteca")},
        {QStringLiteral("Privacy"), QStringLiteral("Privacidade")},
        {QStringLiteral("About"), QStringLiteral("Sobre")},
        {QStringLiteral("Reading comfort"), QStringLiteral("Conforto de leitura")},
        {QStringLiteral("Save reading preferences to"), QStringLiteral("Salvar preferências de leitura em")},
        {QStringLiteral("Open a document to save preferences for a format, content type, or document."), QStringLiteral("Abra um documento para salvar preferências por formato, tipo de conteúdo ou documento.")},
        {QStringLiteral("Theme"), QStringLiteral("Tema")},
        {QStringLiteral("Light"), QStringLiteral("Claro")},
        {QStringLiteral("Dark"), QStringLiteral("Escuro")},
        {QStringLiteral("Sepia"), QStringLiteral("Sépia")},
        {QStringLiteral("Paper"), QStringLiteral("Papel")},
        {QStringLiteral("Gray"), QStringLiteral("Cinza")},
        {QStringLiteral("High contrast"), QStringLiteral("Alto contraste")},
        {QStringLiteral("Low light"), QStringLiteral("Pouca luz")},
        {QStringLiteral("Font size"), QStringLiteral("Tamanho da fonte")},
        {QStringLiteral("Line spacing"), QStringLiteral("Espaçamento entre linhas")},
        {QStringLiteral("Font"), QStringLiteral("Fonte")},
        {QStringLiteral("Text width"), QStringLiteral("Largura do texto")},
        {QStringLiteral("Page margins"), QStringLiteral("Margens da página")},
        {QStringLiteral("Alignment"), QStringLiteral("Alinhamento")},
        {QStringLiteral("Left"), QStringLiteral("Esquerda")},
        {QStringLiteral("Centered"), QStringLiteral("Centralizado")},
        {QStringLiteral("Right"), QStringLiteral("Direita")},
        {QStringLiteral("Justified"), QStringLiteral("Justificado")},
        {QStringLiteral("Reading direction"), QStringLiteral("Direção de leitura")},
        {QStringLiteral("Left to right"), QStringLiteral("Da esquerda para a direita")},
        {QStringLiteral("Right to left"), QStringLiteral("Da direita para a esquerda")},
        {QStringLiteral("Image fit"), QStringLiteral("Ajuste da imagem")},
        {QStringLiteral("Fit page"), QStringLiteral("Ajustar à página")},
        {QStringLiteral("Fit width"), QStringLiteral("Ajustar à largura")},
        {QStringLiteral("Fit height"), QStringLiteral("Ajustar à altura")},
        {QStringLiteral("Actual size"), QStringLiteral("Tamanho real")},
        {QStringLiteral("Reader background"), QStringLiteral("Fundo do leitor")},
        {QStringLiteral("Reader text"), QStringLiteral("Texto do leitor")},
        {QStringLiteral("Highlight color"), QStringLiteral("Cor do destaque")},
        {QStringLiteral("Use continuous reading when available"), QStringLiteral("Usar leitura contínua quando disponível")},
        {QStringLiteral("High contrast controls"), QStringLiteral("Controles de alto contraste")},
        {QStringLiteral("Speed"), QStringLiteral("Velocidade")},
        {QStringLiteral("Volume"), QStringLiteral("Volume")},
        {QStringLiteral("Your preferences are stored locally with your Lumina Reader library."), QStringLiteral("Suas preferências são armazenadas localmente com a biblioteca do Lumina Reader.")},
        {QStringLiteral("Restore database"), QStringLiteral("Restaurar banco de dados")},
        {QStringLiteral("Restore library"), QStringLiteral("Restaurar biblioteca")},
        {QStringLiteral("Export preferences"), QStringLiteral("Exportar preferências")},
        {QStringLiteral("Restore preferences"), QStringLiteral("Restaurar preferências")},
        {QStringLiteral("New collection"), QStringLiteral("Nova coleção")},
        {QStringLiteral("General"), QStringLiteral("Geral")},
        {QStringLiteral("Reading profile"), QStringLiteral("Perfil de leitura")},
        {QStringLiteral("Keep a separate set of reading preferences for each profile."), QStringLiteral("Mantenha um conjunto separado de preferências de leitura para cada perfil.")},
        {QStringLiteral("New profile"), QStringLiteral("Novo perfil")},
        {QStringLiteral("Default"), QStringLiteral("Padrão")},
        {QStringLiteral("Reading profile deleted."), QStringLiteral("Perfil de leitura excluído.")},
        {QStringLiteral("The default profile cannot be deleted."), QStringLiteral("O perfil padrão não pode ser excluído.")},
        {QStringLiteral("Open to Your library on startup"), QStringLiteral("Abrir na sua biblioteca ao iniciar")},
        {QStringLiteral("Show your library when Lumina Reader starts."), QStringLiteral("Mostrar sua biblioteca quando o Lumina Reader iniciar.")},
        {QStringLiteral("Remember reading position"), QStringLiteral("Lembrar posição de leitura")},
        {QStringLiteral("Reopen books at the last position you read."), QStringLiteral("Reabrir livros na última posição lida.")},
        {QStringLiteral("Check for updates automatically"), QStringLiteral("Verificar atualizações automaticamente")},
        {QStringLiteral("Get the latest improvements and fixes."), QStringLiteral("Receba as melhorias e correções mais recentes.")},
        {QStringLiteral("Make every document comfortable to read."), QStringLiteral("Deixe qualquer documento confortável para ler.")},
        {QStringLiteral("Listen to your document while you read along."), QStringLiteral("Ouça seu documento enquanto acompanha a leitura.")},
        {QStringLiteral("Choose how Lumina Reader looks while you read."), QStringLiteral("Escolha a aparência do Lumina Reader durante a leitura.")},
        {QStringLiteral("Interface theme"), QStringLiteral("Tema da interface")},
        {QStringLiteral("The theme is applied immediately to the whole application."), QStringLiteral("O tema é aplicado imediatamente a todo o aplicativo.")},
        {QStringLiteral("Reader colors"), QStringLiteral("Cores do leitor")},
        {QStringLiteral("Use a six-digit color such as #F5F1E8, or leave a field empty to use the theme default."), QStringLiteral("Use uma cor de seis dígitos, como #F5F1E8, ou deixe o campo vazio para usar o padrão do tema.")},
        {QStringLiteral("Manage your documents, collections, and local backups."), QStringLiteral("Gerencie seus documentos, coleções e backups locais.")},
        {QStringLiteral("Your documents stay where you put them. Lumina Reader only stores the library information needed to organize and reopen them."), QStringLiteral("Seus documentos permanecem onde você os colocou. O Lumina Reader armazena apenas as informações necessárias para organizá-los e reabri-los.")},
        {QStringLiteral("Backups and transfer"), QStringLiteral("Backups e transferência")},
        {QStringLiteral("Create a safe copy of your library or move your preferences to another device."), QStringLiteral("Crie uma cópia segura da sua biblioteca ou leve suas preferências para outro dispositivo.")},
        {QStringLiteral("Your reading space stays under your control."), QStringLiteral("Seu espaço de leitura continua sob seu controle.")},
        {QStringLiteral("Lumina Reader keeps your library, reading progress, and saved passages on this device."), QStringLiteral("O Lumina Reader mantém sua biblioteca, seu progresso de leitura e seus trechos salvos neste dispositivo.")},
        {QStringLiteral("Online lookups"), QStringLiteral("Consultas online")},
        {QStringLiteral("Dictionary, translation, and web searches only happen when you explicitly request them. Lumina Reader does not send your library in the background."), QStringLiteral("Consultas de dicionário, tradução e web só acontecem quando você as solicita. O Lumina Reader não envia sua biblioteca em segundo plano.")},
        {QStringLiteral("A calm, private home for your reading library."), QStringLiteral("Um espaço tranquilo e privado para sua biblioteca de leitura.")},
        {QStringLiteral("Version %1"), QStringLiteral("Versão %1")},
        {QStringLiteral("Read, organize, and return to your documents without distractions. Your library remains local to your device by default."), QStringLiteral("Leia, organize e volte aos seus documentos sem distrações. Sua biblioteca permanece local no dispositivo por padrão.")},
        {QStringLiteral("Open source project"), QStringLiteral("Projeto de código aberto")},
        {QStringLiteral("Lumina Reader is built to be transparent, useful, and respectful of your files."), QStringLiteral("O Lumina Reader foi criado para ser transparente, útil e respeitoso com seus arquivos.")},
        {QStringLiteral("Application language"), QStringLiteral("Idioma do aplicativo")},
        {QStringLiteral("Use the Windows or Linux language automatically, or choose another language."), QStringLiteral("Use automaticamente o idioma do Windows ou Linux, ou escolha outro idioma.")},
        {QStringLiteral("Use system language — %1"), QStringLiteral("Usar idioma do sistema — %1")},
        {QStringLiteral("Detected by your operating system: %1"), QStringLiteral("Detectado pelo seu sistema operacional: %1")},
        {QStringLiteral("Interface translations are bundled locally."), QStringLiteral("As traduções da interface estão disponíveis localmente.")},
    };
    return translations;
}

QString translationCatalogKey(const QString &locale)
{
    return locale.trimmed().toLower().replace(QLatin1Char('_'), QLatin1Char('-'));
}

using TranslationCatalogs = QHash<QString, QHash<QString, QString>>;

const TranslationCatalogs &generatedTranslationCatalogs()
{
    static const TranslationCatalogs catalogs = [] {
        TranslationCatalogs result;
        QFile file(QStringLiteral(":/qt/qml/LuminaReader/lumina-locales.txt"));
        if (!file.open(QIODevice::ReadOnly)) {
            return result;
        }

        const QByteArray encoded = file.readAll().simplified().replace(' ', QByteArray());
        const QByteArray compressed = QByteArray::fromBase64(encoded);
        const QByteArray json = qUncompress(compressed);
        const QJsonDocument document = QJsonDocument::fromJson(json);
        const QJsonObject catalogObjects = document.object().value(QStringLiteral("catalogs"))
                                               .toObject();
        for (auto catalog = catalogObjects.constBegin(); catalog != catalogObjects.constEnd();
             ++catalog) {
            const QJsonObject appTranslations = catalog.value().toObject().value(QStringLiteral("app"))
                                                   .toObject();
            QHash<QString, QString> translations;
            for (auto translation = appTranslations.constBegin();
                 translation != appTranslations.constEnd();
                 ++translation) {
                translations.insert(translation.key(), translation.value().toString());
            }
            if (!translations.isEmpty()) {
                result.insert(catalog.key().toLower(), translations);
            }
        }
        return result;
    }();
    return catalogs;
}

const QHash<QString, QString> *translationsForLocale(const QString &locale)
{
    if (locale.compare(QStringLiteral("pt-BR"), Qt::CaseInsensitive) == 0) {
        return &portugueseTranslations();
    }

    const auto &catalogs = generatedTranslationCatalogs();
    const auto iterator = catalogs.constFind(translationCatalogKey(locale));
    return iterator == catalogs.constEnd() ? nullptr : &iterator.value();
}

QString restoreCatalogPlaceholders(const QString &sourceText, QString translation)
{
    if (!sourceText.contains(QRegularExpression(QStringLiteral(R"(%\d+)")))) {
        return translation;
    }

    // Some machine-translated catalogs also translate the protected marker
    // itself (for example, Serbian and Konkani). Restore any marker carrying
    // its original numeric suffix before QML applies QString::arg().
    const QRegularExpression translatedTokenPattern(
        QStringLiteral(R"((?:_LUMINA_TOKEN_|__[^\s_]+_(?:TOKEN|token|टोकन|टोकेन|ТОКЕН)_)(\d+)__)"));
    while (true) {
        const QRegularExpressionMatch match = translatedTokenPattern.match(translation);
        if (!match.hasMatch()) {
            break;
        }

        bool ok = false;
        const int zeroBasedIndex = match.captured(1).toInt(&ok);
        if (!ok) {
            break;
        }

        translation.replace(
            match.capturedStart(),
            match.capturedLength(),
            QStringLiteral("%1").arg(zeroBasedIndex + 1));
    }
    return translation;
}

} // namespace

QString Lumina::Application::LuminaTranslator::translate(const char *context,
                                                          const char *sourceText,
                                                          const char *disambiguation,
                                                          int n) const
{
    Q_UNUSED(context)
    Q_UNUSED(disambiguation)
    Q_UNUSED(n)
    if (sourceText == nullptr) {
        return {};
    }
    const auto *translations = translationsForLocale(locale_);
    if (translations == nullptr) {
        return {};
    }
    const QString source = QString::fromUtf8(sourceText);
    // The product name is a brand, so it must remain stable while the UI
    // around it changes language.
    if (source == QStringLiteral("Lumina Reader")) {
        return source;
    }
    return restoreCatalogPlaceholders(source, translations->value(source));
}

bool Lumina::Application::LuminaTranslator::isEmpty() const
{
    const auto *translations = translationsForLocale(locale_);
    return translations == nullptr || translations->isEmpty();
}

namespace Lumina::Application {

LanguageController::LanguageController(SettingsController *settings, QObject *parent)
    : QObject(parent)
    , settings_(settings)
{
    systemLocale_ = effectiveLocaleFor(QStringLiteral("system"));
    systemLanguageLabel_ = labelFor(systemLocale_);
    if (settings_ != nullptr && !settings_->languageLocale().trimmed().isEmpty()) {
        selectedLocale_ = settings_->languageLocale();
    }
    if (selectedLocale_.compare(QStringLiteral("system"), Qt::CaseInsensitive) == 0) {
        selectedLocale_ = QStringLiteral("system");
    } else {
        const QString normalized = normalizedLocale(selectedLocale_);
        if (availableLocales().contains(normalized)) {
            selectedLocale_ = normalized;
        } else {
            selectedLocale_ = QStringLiteral("system");
        }
    }
    applyLanguage();
}

LanguageController::~LanguageController()
{
    if (translatorInstalled_) {
        QCoreApplication::removeTranslator(&translator_);
    }
}

QString LanguageController::selectedLocale() const
{
    return selectedLocale_;
}

QString LanguageController::locale() const
{
    return effectiveLocale_;
}

QString LanguageController::systemLocale() const
{
    return systemLocale_;
}

QString LanguageController::systemLanguageLabel() const
{
    return systemLanguageLabel_;
}

QStringList LanguageController::availableLocales() const
{
    QStringList locales;
    locales.reserve(static_cast<int>(sizeof(kLanguages) / sizeof(kLanguages[0])));
    for (const LanguageEntry &entry : kLanguages) {
        locales.append(QString::fromLatin1(entry.locale));
    }
    return locales;
}

QStringList LanguageController::availableLanguageLabels() const
{
    QStringList labels;
    labels.reserve(static_cast<int>(sizeof(kLanguages) / sizeof(kLanguages[0])) + 1);
    labels.append(QCoreApplication::translate("LanguageController", "Use system language — %1")
                      .arg(systemLanguageLabel_));
    QSet<QString> seenLabels;
    for (const LanguageEntry &entry : kLanguages) {
        if (isCompatibilityAlias(entry)) {
            continue;
        }

        const QString label = QString::fromUtf8(entry.label);
        if (seenLabels.contains(label)) {
            continue;
        }
        seenLabels.insert(label);
        labels.append(label);
    }
    return labels;
}

QStringList LanguageController::availableSelectionLocales() const
{
    QStringList locales;
    locales.reserve(static_cast<int>(sizeof(kLanguages) / sizeof(kLanguages[0])));
    QSet<QString> seenLabels;
    for (const LanguageEntry &entry : kLanguages) {
        if (isCompatibilityAlias(entry)) {
            continue;
        }

        const QString label = QString::fromUtf8(entry.label);
        if (seenLabels.contains(label)) {
            continue;
        }
        seenLabels.insert(label);
        locales.append(QString::fromLatin1(entry.locale));
    }
    return locales;
}

bool LanguageController::usesSystemLanguage() const
{
    return selectedLocale_.compare(QStringLiteral("system"), Qt::CaseInsensitive) == 0;
}

bool LanguageController::translationAvailable() const
{
    return effectiveLocale_.compare(QStringLiteral("en-US"), Qt::CaseInsensitive) == 0
        || translationsForLocale(effectiveLocale_) != nullptr;
}

QString LanguageController::translationStatus() const
{
    if (effectiveLocale_.compare(QStringLiteral("en-US"), Qt::CaseInsensitive) == 0) {
        return QCoreApplication::translate("LanguageController", "English interface is built in.");
    }
    return QCoreApplication::translate(
        "LanguageController", "Interface translations are bundled locally.");
}

bool LanguageController::rightToLeft() const
{
    return isRightToLeftLanguage(effectiveLocale_);
}

void LanguageController::setQmlEngine(QQmlEngine *engine)
{
    qmlEngine_ = engine;
}

bool LanguageController::setLanguage(const QString &locale)
{
    const QString normalized = locale.trimmed().compare(QStringLiteral("system"), Qt::CaseInsensitive) == 0
        ? QStringLiteral("system")
        : normalizedLocale(locale);
    if (normalized != QStringLiteral("system") && !availableLocales().contains(normalized)) {
        return false;
    }
    if (normalized == selectedLocale_) {
        return true;
    }

    selectedLocale_ = normalized;
    if (settings_ != nullptr) {
        settings_->setLanguageLocale(selectedLocale_);
    }
    applyLanguage();
    return true;
}

QString LanguageController::labelForLocale(const QString &locale) const
{
    return labelFor(locale);
}

QString LanguageController::displayLocaleFor(const QString &locale) const
{
    return displayLocaleForValue(locale);
}

QString LanguageController::formatInteger(double value) const
{
    if (!std::isfinite(value)) {
        return QStringLiteral("0");
    }

    const double rounded = std::round(value);
    if (rounded > static_cast<double>(std::numeric_limits<qlonglong>::max())
        || rounded < static_cast<double>(std::numeric_limits<qlonglong>::min())) {
        return QStringLiteral("0");
    }

    return formattingLocale().toString(static_cast<qlonglong>(rounded));
}

QString LanguageController::formatDecimal(double value, int fractionDigits) const
{
    if (!std::isfinite(value)) {
        return QStringLiteral("0");
    }

    const int digits = std::clamp(fractionDigits, 0, 6);
    return formattingLocale().toString(value, 'f', digits);
}

QString LanguageController::formatPercent(double value, int fractionDigits) const
{
    return formatDecimal(value, fractionDigits) + formattingLocale().percent();
}

int LanguageController::parseInteger(const QString &value) const
{
    bool ok = false;
    const qlonglong parsed = formattingLocale().toLongLong(value.trimmed(), &ok);
    if (!ok) {
        return 0;
    }

    return static_cast<int>(std::clamp<qlonglong>(
        parsed,
        std::numeric_limits<int>::min(),
        std::numeric_limits<int>::max()));
}

void LanguageController::applyLanguage()
{
    effectiveLocale_ = effectiveLocaleFor(selectedLocale_);
    QLocale::setDefault(usesSystemLanguage() ? QLocale::system() : QLocale(effectiveLocale_));
    if (qobject_cast<QGuiApplication *>(QCoreApplication::instance()) != nullptr) {
        QGuiApplication::setLayoutDirection(
            rightToLeft() ? Qt::RightToLeft : Qt::LeftToRight);
    }
    if (translatorInstalled_) {
        QCoreApplication::removeTranslator(&translator_);
        translatorInstalled_ = false;
    }

    translator_.setLocale(effectiveLocale_);
    if (effectiveLocale_.compare(QStringLiteral("en-US"), Qt::CaseInsensitive) != 0
        && translationsForLocale(effectiveLocale_) != nullptr) {
        translatorInstalled_ = QCoreApplication::installTranslator(&translator_);
    }

    emit languageChanged();
    if (qmlEngine_ != nullptr) {
        QMetaObject::invokeMethod(qmlEngine_, "retranslate");
    }
}

QString LanguageController::effectiveLocaleFor(const QString &selection) const
{
    if (selection.compare(QStringLiteral("system"), Qt::CaseInsensitive) == 0) {
        const QStringList candidates = uiLanguageCandidates();
        for (const QString &candidate : candidates) {
            const QString normalized = normalizedLocale(candidate);
            if (availableLocales().contains(normalized)) {
                return normalized;
            }
            const QString language = languagePart(normalized);
            for (const QString &available : availableLocales()) {
                if (languagePart(available) == language) {
                    return available;
                }
            }
        }
        return QStringLiteral("en-US");
    }
    const QString normalized = normalizedLocale(selection);
    if (availableLocales().contains(normalized)) {
        return normalized;
    }
    return QStringLiteral("en-US");
}

QString LanguageController::normalizedLocale(const QString &locale) const
{
    return canonicalLocale(locale);
}

QStringList LanguageController::uiLanguageCandidates() const
{
    QStringList candidates;
    const QLocale system = QLocale::system();
    candidates.append(system.uiLanguages());
    candidates.append(system.bcp47Name());
    candidates.append(system.name());
    candidates.removeAll(QString());
    candidates.removeDuplicates();
    return candidates;
}

QLocale LanguageController::formattingLocale() const
{
    return usesSystemLanguage() ? QLocale::system() : QLocale(effectiveLocale_);
}

} // namespace Lumina::Application
