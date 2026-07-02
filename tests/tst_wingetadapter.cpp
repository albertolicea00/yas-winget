#include <QTest>

#include "wingetadapter.h"

class TestWingetAdapter : public QObject {
    Q_OBJECT
private slots:
    void searchParsesColumnTable()
    {
        WingetAdapter adapter;
        const auto packages = adapter.parseSearch(QStringLiteral(
            "Name               Id                     Version  Match        Source\n"
            "--------------------------------------------------------------------\n"
            "7-Zip              7zip.7zip              25.00                 winget\n"
            "Visual Studio Code Microsoft.VisualStudioCode 1.90 Tag: editor winget\n"));
        QCOMPARE(packages.size(), 2);
        QCOMPARE(packages.at(0).id, QStringLiteral("7zip.7zip"));
        QCOMPARE(packages.at(0).name, QStringLiteral("7-Zip"));
        QCOMPARE(packages.at(0).version, QStringLiteral("25.00"));
        QCOMPARE(packages.at(0).source, QStringLiteral("winget"));
    }

    void upgradeParsesAvailableColumn()
    {
        WingetAdapter adapter;
        const auto packages = adapter.parseOutdated(QStringLiteral(
            "Name   Id          Version Available Source\n"
            "--------------------------------------------\n"
            "7-Zip  7zip.7zip   24.08   25.00     winget\n"));
        QCOMPARE(packages.size(), 1);
        QCOMPARE(packages.at(0).installedVersion, QStringLiteral("24.08"));
        QCOMPARE(packages.at(0).version, QStringLiteral("25.00"));
        QVERIFY(packages.at(0).outdated());
    }

    void listSkipsRowsWithoutAvailableWhenParsingOutdated()
    {
        WingetAdapter adapter;
        const auto packages = adapter.parseOutdated(QStringLiteral(
            "Name   Id          Version Available Source\n"
            "--------------------------------------------\n"
            "Foo    Foo.Foo     1.0               winget\n"));
        QCOMPARE(packages.size(), 0);
    }

    void installCommandUsesExactId()
    {
        WingetAdapter adapter;
        const auto cmd = adapter.installCommand("7zip.7zip", "");
        QCOMPARE(cmd.program, QStringLiteral("winget"));
        QVERIFY(cmd.arguments.contains(QStringLiteral("--id")));
        QVERIFY(cmd.arguments.contains(QStringLiteral("-e")));
        QVERIFY(cmd.arguments.contains(QStringLiteral("--disable-interactivity")));
    }
};

QTEST_MAIN(TestWingetAdapter)
#include "tst_wingetadapter.moc"
