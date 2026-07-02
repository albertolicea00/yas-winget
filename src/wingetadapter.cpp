#include "wingetadapter.h"

#include <QHash>

using yas::CliAction;
using yas::CliCommand;
using yas::Package;

// winget has no machine-readable list output (JSON only via COM API, see
// CLAUDE.md) — tables are parsed by header column positions. Values truncated
// by winget keep their trailing ellipsis.
namespace {

const QString kWinget = QStringLiteral("winget");

QStringList quiet()
{
    return {QStringLiteral("--disable-interactivity"),
            QStringLiteral("--accept-source-agreements")};
}

} // namespace

// Winget prints fixed-width tables; cells are sliced by the header's column
// start positions. Small state machine: header -> dashed separator -> body.
static QList<QHash<QString, QString>> parseWingetTable(const QString &stdOut)
{
    QList<QHash<QString, QString>> rows;
    const QStringList lines = stdOut.split(QLatin1Char('\n'));

    struct Col { QString title; qsizetype start; qsizetype end; };
    QList<Col> columns;
    enum { SeekHeader, SeekSeparator, Body } state = SeekHeader;

    for (const QString &line : lines) {
        switch (state) {
        case SeekHeader: {
            if (!line.contains(QStringLiteral("Name")) || !line.contains(QStringLiteral(" Id")))
                break;
            static const QStringList known = {
                QStringLiteral("Name"), QStringLiteral("Id"), QStringLiteral("Version"),
                QStringLiteral("Available"), QStringLiteral("Match"), QStringLiteral("Source"),
            };
            for (const QString &title : known) {
                const qsizetype pos = line.indexOf(title);
                if (pos >= 0)
                    columns.append({title.toLower(), pos, -1});
            }
            std::sort(columns.begin(), columns.end(),
                      [](const Col &a, const Col &b) { return a.start < b.start; });
            for (qsizetype i = 0; i < columns.size(); ++i)
                columns[i].end = (i + 1 < columns.size()) ? columns[i + 1].start : -1;
            state = SeekSeparator;
            break;
        }
        case SeekSeparator:
            if (line.startsWith(QLatin1Char('-')))
                state = Body;
            break;
        case Body: {
            if (line.trimmed().isEmpty())
                break;
            QHash<QString, QString> row;
            for (const Col &c : columns) {
                const QString cell = (c.end < 0) ? line.mid(c.start)
                                                 : line.mid(c.start, c.end - c.start);
                row.insert(c.title, cell.trimmed());
            }
            if (!row.value(QStringLiteral("id")).isEmpty())
                rows.append(row);
            break;
        }
        }
    }
    return rows;
}

QString WingetAdapter::displayName() const { return QStringLiteral("Windows Package Manager"); }
QString WingetAdapter::cliProgram() const { return kWinget; }
QStringList WingetAdapter::cliSearchPaths() const { return {}; }
QStringList WingetAdapter::cliVersionArguments() const { return {QStringLiteral("--version")}; }

CliCommand WingetAdapter::searchCommand(const QString &query) const
{
    return {kWinget, QStringList{QStringLiteral("search"), query} + quiet()};
}

CliCommand WingetAdapter::infoCommand(const QString &packageId, const QString &) const
{
    return {kWinget, QStringList{QStringLiteral("show"), QStringLiteral("--id"), packageId,
                                 QStringLiteral("-e")} + quiet()};
}

CliCommand WingetAdapter::listInstalledCommand() const
{
    return {kWinget, QStringList{QStringLiteral("list")} + quiet()};
}

CliCommand WingetAdapter::listOutdatedCommand() const
{
    return {kWinget, QStringList{QStringLiteral("upgrade")} + quiet()};
}

CliCommand WingetAdapter::installCommand(const QString &packageId, const QString &) const
{
    return {kWinget, QStringList{QStringLiteral("install"), QStringLiteral("--id"), packageId,
                                 QStringLiteral("-e"),
                                 QStringLiteral("--accept-package-agreements")} + quiet()};
}

CliCommand WingetAdapter::uninstallCommand(const QString &packageId, const QString &) const
{
    return {kWinget, QStringList{QStringLiteral("uninstall"), QStringLiteral("--id"),
                                 packageId, QStringLiteral("-e")} + quiet()};
}

CliCommand WingetAdapter::upgradeCommand(const QString &packageId, const QString &) const
{
    return {kWinget, QStringList{QStringLiteral("upgrade"), QStringLiteral("--id"), packageId,
                                 QStringLiteral("-e"),
                                 QStringLiteral("--accept-package-agreements")} + quiet()};
}

CliCommand WingetAdapter::upgradeAllCommand() const
{
    return {kWinget, QStringList{QStringLiteral("upgrade"), QStringLiteral("--all"),
                                 QStringLiteral("--accept-package-agreements")} + quiet()};
}

CliCommand WingetAdapter::pinCommand(const QString &packageId, const QString &) const
{
    return {kWinget, {QStringLiteral("pin"), QStringLiteral("add"), QStringLiteral("--id"),
                      packageId, QStringLiteral("-e")}};
}

CliCommand WingetAdapter::unpinCommand(const QString &packageId, const QString &) const
{
    return {kWinget, {QStringLiteral("pin"), QStringLiteral("remove"), QStringLiteral("--id"),
                      packageId, QStringLiteral("-e")}};
}

QList<Package> WingetAdapter::parseSearch(const QString &stdOut) const
{
    QList<Package> result;
    const auto rows = parseWingetTable(stdOut);
    for (const auto &row : rows) {
        Package p;
        p.id = row.value(QStringLiteral("id"));
        p.name = row.value(QStringLiteral("name"));
        p.version = row.value(QStringLiteral("version"));
        p.source = row.value(QStringLiteral("source"));
        result.append(p);
    }
    return result;
}

QList<Package> WingetAdapter::parseInfo(const QString &stdOut) const
{
    // "Found <Name> [<Id>]" then "Key: Value" lines.
    Package p;
    const QStringList lines = stdOut.split(QLatin1Char('\n'));
    for (const QString &raw : lines) {
        const QString line = raw.trimmed();
        if (line.startsWith(QStringLiteral("Found "))) {
            const qsizetype bracket = line.lastIndexOf(QLatin1Char('['));
            if (bracket > 0) {
                p.name = line.mid(6, bracket - 6).trimmed();
                p.id = line.mid(bracket + 1).remove(QLatin1Char(']')).trimmed();
            }
            continue;
        }
        const qsizetype colon = line.indexOf(QLatin1Char(':'));
        if (colon <= 0)
            continue;
        const QString key = line.left(colon).trimmed().toLower();
        const QString value = line.mid(colon + 1).trimmed();
        if (key == QStringLiteral("version")) p.version = value;
        else if (key == QStringLiteral("description")) p.description = value;
        else if (key == QStringLiteral("homepage")) p.homepage = value;
        else if (key == QStringLiteral("publisher") && p.description.isEmpty())
            p.description = value;
    }
    if (p.id.isEmpty())
        return {};
    return {p};
}

QList<Package> WingetAdapter::parseInstalled(const QString &stdOut) const
{
    QList<Package> result;
    const auto rows = parseWingetTable(stdOut);
    for (const auto &row : rows) {
        Package p;
        p.id = row.value(QStringLiteral("id"));
        p.name = row.value(QStringLiteral("name"));
        p.installedVersion = row.value(QStringLiteral("version"));
        const QString available = row.value(QStringLiteral("available"));
        p.version = available.isEmpty() ? p.installedVersion : available;
        p.source = row.value(QStringLiteral("source"));
        result.append(p);
    }
    return result;
}

QList<Package> WingetAdapter::parseOutdated(const QString &stdOut) const
{
    QList<Package> result;
    const auto rows = parseWingetTable(stdOut);
    for (const auto &row : rows) {
        const QString available = row.value(QStringLiteral("available"));
        if (available.isEmpty())
            continue;
        Package p;
        p.id = row.value(QStringLiteral("id"));
        p.name = row.value(QStringLiteral("name"));
        p.installedVersion = row.value(QStringLiteral("version"));
        p.version = available;
        p.source = row.value(QStringLiteral("source"));
        result.append(p);
    }
    return result;
}

QList<CliAction> WingetAdapter::actionCatalog() const
{
    return {
        {QStringLiteral("source-update"), tr("Update sources"),
         tr("Refresh package metadata from all sources"),
         {kWinget, {QStringLiteral("source"), QStringLiteral("update")}}, false, false, true},
        {QStringLiteral("sources"), tr("List sources"),
         tr("Show the configured package sources (winget, msstore...)"),
         {kWinget, {QStringLiteral("source"), QStringLiteral("list")}}, false, false, false},
        {QStringLiteral("pins"), tr("List pins"),
         tr("Show all pinned packages"),
         {kWinget, {QStringLiteral("pin"), QStringLiteral("list")}}, false, false, false},
        {QStringLiteral("info"), tr("Winget diagnostics"),
         tr("Show winget version, paths and system information"),
         {kWinget, {QStringLiteral("--info")}}, false, false, false},
    };
}
