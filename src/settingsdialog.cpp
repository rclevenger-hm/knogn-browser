#include "settingsdialog.h"

#include "appsettings.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTabWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace {

class SettingsDialog final : public QDialog {
public:
    explicit SettingsDialog(QWidget *parent = nullptr)
        : QDialog(parent) {
        setWindowTitle(QStringLiteral("Knogn Settings"));
        resize(650, 530);

        auto *root = new QVBoxLayout(this);
        auto *tabs = new QTabWidget(this);
        root->addWidget(tabs, 1);

        buildGeneralTab(tabs);
        buildPrivacyTab(tabs);
        buildNetworkTab(tabs);
        buildMediaTab(tabs);

        auto *buttons = new QDialogButtonBox(
            QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
        root->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, this, [this] { save(); });
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

private:
    void buildGeneralTab(QTabWidget *tabs) {
        auto *page = new QWidget(tabs);
        auto *form = new QFormLayout(page);

        search_ = new QComboBox(page);
        search_->addItem(QStringLiteral("DuckDuckGo"),
                         QStringLiteral("https://duckduckgo.com/?q=%1"));
        search_->addItem(QStringLiteral("Brave Search"),
                         QStringLiteral("https://search.brave.com/search?q=%1"));
        search_->addItem(QStringLiteral("Startpage"),
                         QStringLiteral("https://www.startpage.com/sp/search?query=%1"));
        search_->addItem(QStringLiteral("Custom"), QStringLiteral("custom"));
        form->addRow(QStringLiteral("Search engine"), search_);

        customSearch_ = new QLineEdit(page);
        customSearch_->setPlaceholderText(
            QStringLiteral("https://example.com/search?q=%1"));
        form->addRow(QStringLiteral("Custom search URL"), customSearch_);

        homePage_ = new QLineEdit(page);
        homePage_->setPlaceholderText(QStringLiteral("about:blank"));
        form->addRow(QStringLiteral("Home page"), homePage_);

        askDownloads_ = new QCheckBox(
            QStringLiteral("Ask where to save each download"), page);
        form->addRow(QString(), askDownloads_);

        auto *downloadRow = new QWidget(page);
        auto *downloadLayout = new QHBoxLayout(downloadRow);
        downloadLayout->setContentsMargins(0, 0, 0, 0);
        downloadDirectory_ = new QLineEdit(downloadRow);
        auto *browse = new QPushButton(QStringLiteral("Browse…"), downloadRow);
        downloadLayout->addWidget(downloadDirectory_, 1);
        downloadLayout->addWidget(browse);
        form->addRow(QStringLiteral("Download folder"), downloadRow);
        connect(browse, &QPushButton::clicked, this, [this] {
            const QString path = QFileDialog::getExistingDirectory(
                this, QStringLiteral("Choose download folder"), downloadDirectory_->text());
            if (!path.isEmpty()) downloadDirectory_->setText(path);
        });

        const QString currentSearch = AppSettings::searchTemplate();
        int selected = -1;
        for (int i = 0; i < search_->count() - 1; ++i) {
            if (search_->itemData(i).toString() == currentSearch) {
                selected = i;
                break;
            }
        }
        if (selected >= 0) {
            search_->setCurrentIndex(selected);
        } else {
            search_->setCurrentIndex(search_->count() - 1);
            customSearch_->setText(currentSearch);
        }
        customSearch_->setEnabled(search_->currentData().toString() == QStringLiteral("custom"));
        connect(search_, &QComboBox::currentIndexChanged, this, [this] {
            customSearch_->setEnabled(search_->currentData().toString() == QStringLiteral("custom"));
        });

        homePage_->setText(AppSettings::homePage());
        askDownloads_->setChecked(AppSettings::askWhereToSaveDownloads());
        downloadDirectory_->setText(AppSettings::downloadDirectory());
        tabs->addTab(page, QStringLiteral("General"));
    }

    void buildPrivacyTab(QTabWidget *tabs) {
        auto *page = new QWidget(tabs);
        auto *layout = new QVBoxLayout(page);

        blockThirdParty_ = new QCheckBox(
            QStringLiteral("Block third-party cookies and site storage"), page);
        dnt_ = new QCheckBox(QStringLiteral("Send Do Not Track (DNT)"), page);
        gpc_ = new QCheckBox(QStringLiteral("Send Global Privacy Control (GPC)"), page);
        spellCheck_ = new QCheckBox(QStringLiteral("Enable local spell checking"), page);
        passwords_ = new QCheckBox(
            QStringLiteral("Enable Knogn password manager"), page);
        autoFill_ = new QCheckBox(
            QStringLiteral("Auto-fill saved logins when a matching site loads"), page);

        blockThirdParty_->setChecked(AppSettings::blockThirdPartyState());
        dnt_->setChecked(AppSettings::sendDoNotTrack());
        gpc_->setChecked(AppSettings::sendGlobalPrivacyControl());
        spellCheck_->setChecked(AppSettings::spellCheckEnabled());
        passwords_->setChecked(AppSettings::passwordManagerEnabled());
        autoFill_->setChecked(AppSettings::autoFillPasswords());

        layout->addWidget(blockThirdParty_);
        layout->addWidget(dnt_);
        layout->addWidget(gpc_);
        layout->addWidget(spellCheck_);
        layout->addSpacing(16);
        layout->addWidget(passwords_);
        layout->addWidget(autoFill_);
        auto *note = new QLabel(
            QStringLiteral("Saved secrets are stored through the operating system credential store; Knogn stores only site/user metadata in its settings."),
            page);
        note->setWordWrap(true);
        note->setStyleSheet(QStringLiteral("color:#9bcaaa;"));
        layout->addWidget(note);
        layout->addStretch(1);
        tabs->addTab(page, QStringLiteral("Privacy & Passwords"));
    }

    void buildNetworkTab(QTabWidget *tabs) {
        auto *page = new QWidget(tabs);
        auto *layout = new QVBoxLayout(page);
        auto *info = new QLabel(
            QStringLiteral("Knogn automatically follows an active operating-system VPN. You can also route only Knogn through an HTTP or SOCKS5 tunnel below. Network routing changes apply after restarting Knogn."),
            page);
        info->setWordWrap(true);
        layout->addWidget(info);

        auto *form = new QFormLayout();
        proxyMode_ = new QComboBox(page);
        proxyMode_->addItem(QStringLiteral("System routing / VPN (recommended)"), QStringLiteral("system"));
        proxyMode_->addItem(QStringLiteral("Direct connection"), QStringLiteral("direct"));
        proxyMode_->addItem(QStringLiteral("HTTP proxy / tunnel"), QStringLiteral("http"));
        proxyMode_->addItem(QStringLiteral("SOCKS5 proxy / tunnel"), QStringLiteral("socks5"));
        form->addRow(QStringLiteral("Routing mode"), proxyMode_);

        proxyEndpoint_ = new QLineEdit(page);
        proxyEndpoint_->setPlaceholderText(QStringLiteral("host:port"));
        form->addRow(QStringLiteral("Tunnel endpoint"), proxyEndpoint_);

        proxyBypass_ = new QLineEdit(page);
        proxyBypass_->setPlaceholderText(QStringLiteral("localhost;127.0.0.1;*.lan"));
        form->addRow(QStringLiteral("Bypass list"), proxyBypass_);
        layout->addLayout(form);

        const QString mode = AppSettings::proxyMode();
        const int index = proxyMode_->findData(mode);
        proxyMode_->setCurrentIndex(index >= 0 ? index : 0);
        proxyEndpoint_->setText(AppSettings::proxyEndpoint());
        proxyBypass_->setText(AppSettings::proxyBypassList());
        auto updateEndpointState = [this] {
            const QString modeValue = proxyMode_->currentData().toString();
            proxyEndpoint_->setEnabled(modeValue == QStringLiteral("http") ||
                                       modeValue == QStringLiteral("socks5"));
            proxyBypass_->setEnabled(modeValue != QStringLiteral("direct"));
        };
        updateEndpointState();
        connect(proxyMode_, &QComboBox::currentIndexChanged, this,
                [updateEndpointState] { updateEndpointState(); });

#ifdef Q_OS_WIN
        auto *vpnSettings = new QPushButton(QStringLiteral("Open Windows VPN settings…"), page);
        connect(vpnSettings, &QPushButton::clicked, this, [] {
            QDesktopServices::openUrl(QUrl(QStringLiteral("ms-settings:network-vpn")));
        });
        layout->addWidget(vpnSettings, 0, Qt::AlignLeft);
#endif

        auto *warning = new QLabel(
            QStringLiteral("Do not put VPN/proxy passwords in the endpoint URL. Provider authentication will be integrated with the secure credential store separately."),
            page);
        warning->setWordWrap(true);
        warning->setStyleSheet(QStringLiteral("color:#9bcaaa;"));
        layout->addWidget(warning);
        layout->addStretch(1);
        tabs->addTab(page, QStringLiteral("VPN & Network"));
    }

    void buildMediaTab(QTabWidget *tabs) {
        auto *page = new QWidget(tabs);
        auto *layout = new QVBoxLayout(page);
        auto *title = new QLabel(QStringLiteral("Web media compatibility"), page);
        title->setStyleSheet(QStringLiteral("font-size:18px;font-weight:600;"));
        layout->addWidget(title);
        auto *body = new QLabel(
            QStringLiteral(
                "Hardware acceleration, WebGL, Media Source Extensions and HTML5 fullscreen are enabled. "
                "Codec support still depends on the Qt WebEngine build. Type knogn://media in the address bar to inspect H.264/AAC, AV1, VP9, Widevine and other runtime capabilities.\n\n"
                "Knogn will not label missing H.264/AAC or DRM support as fixed until released binaries actually pass those playback probes."),
            page);
        body->setWordWrap(true);
        layout->addWidget(body);
        layout->addStretch(1);
        tabs->addTab(page, QStringLiteral("Media"));
    }

    void save() {
        QString searchTemplate = search_->currentData().toString();
        if (searchTemplate == QStringLiteral("custom")) {
            searchTemplate = customSearch_->text().trimmed();
            if (!searchTemplate.contains(QStringLiteral("%1"))) {
                QMessageBox::warning(
                    this, QStringLiteral("Invalid search URL"),
                    QStringLiteral("A custom search URL must contain %1 where the encoded search terms should be inserted."));
                return;
            }
        }

        const QString proxyMode = proxyMode_->currentData().toString();
        if ((proxyMode == QStringLiteral("http") || proxyMode == QStringLiteral("socks5")) &&
            proxyEndpoint_->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("Missing tunnel endpoint"),
                                 QStringLiteral("Enter a host and port for the selected routing mode."));
            return;
        }

        QSettings s;
        s.setValue(QStringLiteral("general/searchTemplate"), searchTemplate);
        s.setValue(QStringLiteral("general/homePage"), homePage_->text().trimmed().isEmpty()
                       ? QStringLiteral("about:blank") : homePage_->text().trimmed());
        s.setValue(QStringLiteral("downloads/askEveryTime"), askDownloads_->isChecked());
        s.setValue(QStringLiteral("downloads/directory"), downloadDirectory_->text().trimmed());
        s.setValue(QStringLiteral("privacy/blockThirdPartyState"), blockThirdParty_->isChecked());
        s.setValue(QStringLiteral("privacy/sendDnt"), dnt_->isChecked());
        s.setValue(QStringLiteral("privacy/sendGpc"), gpc_->isChecked());
        s.setValue(QStringLiteral("privacy/spellCheck"), spellCheck_->isChecked());
        s.setValue(QStringLiteral("passwords/enabled"), passwords_->isChecked());
        s.setValue(QStringLiteral("passwords/autoFill"), autoFill_->isChecked());
        s.setValue(QStringLiteral("network/proxyMode"), proxyMode);
        s.setValue(QStringLiteral("network/proxyEndpoint"), proxyEndpoint_->text().trimmed());
        s.setValue(QStringLiteral("network/proxyBypass"), proxyBypass_->text().trimmed());
        s.sync();
        accept();
    }

    QComboBox *search_ = nullptr;
    QLineEdit *customSearch_ = nullptr;
    QLineEdit *homePage_ = nullptr;
    QCheckBox *askDownloads_ = nullptr;
    QLineEdit *downloadDirectory_ = nullptr;
    QCheckBox *blockThirdParty_ = nullptr;
    QCheckBox *dnt_ = nullptr;
    QCheckBox *gpc_ = nullptr;
    QCheckBox *spellCheck_ = nullptr;
    QCheckBox *passwords_ = nullptr;
    QCheckBox *autoFill_ = nullptr;
    QComboBox *proxyMode_ = nullptr;
    QLineEdit *proxyEndpoint_ = nullptr;
    QLineEdit *proxyBypass_ = nullptr;
};

}  // namespace

bool showSettingsDialog(QWidget *parent) {
    SettingsDialog dialog(parent);
    return dialog.exec() == QDialog::Accepted;
}
