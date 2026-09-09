#include "branding.h"

#include <QColor>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

namespace {

QPainterPath upperLeafPath(qreal s) {
    QPainterPath path;
    path.moveTo(0.34 * s, 0.67 * s);
    path.cubicTo(0.29 * s, 0.47 * s, 0.39 * s, 0.29 * s, 0.70 * s, 0.15 * s);
    path.cubicTo(0.78 * s, 0.11 * s, 0.83 * s, 0.08 * s, 0.87 * s, 0.04 * s);
    path.cubicTo(0.84 * s, 0.28 * s, 0.72 * s, 0.44 * s, 0.49 * s, 0.54 * s);
    path.cubicTo(0.41 * s, 0.58 * s, 0.36 * s, 0.62 * s, 0.34 * s, 0.67 * s);
    path.closeSubpath();
    return path;
}

QPainterPath lowerLeafPath(qreal s) {
    QPainterPath path;
    path.moveTo(0.37 * s, 0.70 * s);
    path.cubicTo(0.49 * s, 0.55 * s, 0.64 * s, 0.56 * s, 0.78 * s, 0.67 * s);
    path.cubicTo(0.84 * s, 0.72 * s, 0.89 * s, 0.76 * s, 0.95 * s, 0.76 * s);
    path.cubicTo(0.82 * s, 0.88 * s, 0.67 * s, 0.91 * s, 0.53 * s, 0.83 * s);
    path.cubicTo(0.46 * s, 0.78 * s, 0.41 * s, 0.73 * s, 0.37 * s, 0.70 * s);
    path.closeSubpath();
    return path;
}

}  // namespace

namespace Branding {

QIcon applicationIcon(int size) {
    const int px = qMax(size, 32);
    QPixmap pixmap(px, px);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF canvas(1.0, 1.0, px - 2.0, px - 2.0);
    const qreal radius = px * 0.20;
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(QStringLiteral("#071711")));
    painter.drawRoundedRect(canvas, radius, radius);

    QLinearGradient ring(0, 0, px, px);
    ring.setColorAt(0.0, QColor(QStringLiteral("#DDF5E4")));
    ring.setColorAt(0.45, QColor(QStringLiteral("#9BCBAA")));
    ring.setColorAt(1.0, QColor(QStringLiteral("#397A5D")));
    QPen ringPen(QBrush(ring), px * 0.055, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(ringPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QRectF(px * 0.14, px * 0.14, px * 0.72, px * 0.72));

    painter.save();
    painter.translate(px * 0.08, px * 0.08);
    const qreal s = px * 0.78;

    QLinearGradient upper(0, 0, s, s);
    upper.setColorAt(0.0, QColor(QStringLiteral("#E7F7E9")));
    upper.setColorAt(0.55, QColor(QStringLiteral("#9BCBAA")));
    upper.setColorAt(1.0, QColor(QStringLiteral("#4A8C69")));
    painter.setPen(Qt::NoPen);
    painter.setBrush(upper);
    painter.drawPath(upperLeafPath(s));

    QLinearGradient lower(0, 0, s, s);
    lower.setColorAt(0.0, QColor(QStringLiteral("#CDEBD4")));
    lower.setColorAt(0.55, QColor(QStringLiteral("#7EB493")));
    lower.setColorAt(1.0, QColor(QStringLiteral("#2F6B50")));
    painter.setBrush(lower);
    painter.drawPath(lowerLeafPath(s));

    painter.restore();
    return QIcon(pixmap);
}

QString applicationStyleSheet() {
    return QStringLiteral(R"CSS(
        QMainWindow, QDialog, QMessageBox {
            background: #071711;
            color: #EAF3EC;
        }
        QMenuBar {
            background: #0A2018;
            color: #DDECE1;
            border-bottom: 1px solid #1F4938;
            padding: 3px 8px;
        }
        QMenuBar::item:selected, QMenu::item:selected {
            background: #173B2D;
            border-radius: 5px;
        }
        QMenu {
            background: #0B2119;
            color: #EAF3EC;
            border: 1px solid #285440;
        }
        QToolBar {
            background: #0A1D16;
            border: none;
            border-bottom: 1px solid #1D4434;
            spacing: 6px;
            padding: 7px 9px;
        }
        QToolButton {
            color: #DCEDE1;
            background: transparent;
            border: none;
            border-radius: 7px;
            padding: 6px 8px;
            font-size: 15px;
        }
        QToolButton:hover {
            background: #173B2D;
        }
        QLineEdit {
            background: #102A20;
            color: #F3F8F4;
            border: 1px solid #2C5C46;
            border-radius: 13px;
            padding: 7px 12px;
            selection-background-color: #4C8C69;
        }
        QLineEdit:focus {
            border: 1px solid #8FC2A0;
        }
        QTabWidget::pane {
            border: none;
            background: #071711;
        }
        QTabBar::tab {
            background: #0B2018;
            color: #AFC8B7;
            border: none;
            border-right: 1px solid #18382B;
            min-width: 150px;
            padding: 9px 14px;
        }
        QTabBar::tab:selected {
            background: #143426;
            color: #F0F7F2;
        }
        QTabBar::tab:hover:!selected {
            background: #102B20;
        }
        QStatusBar {
            background: #081B14;
            color: #8FB29C;
            border-top: 1px solid #163729;
        }
        QProgressBar {
            background: #102A20;
            border: 1px solid #285440;
            border-radius: 5px;
            color: transparent;
        }
        QProgressBar::chunk {
            background: #79B18D;
            border-radius: 4px;
        }
    )CSS");
}

QString newTabHtml() {
    return QStringLiteral(R"HTML(
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="color-scheme" content="dark">
<title>New Tab</title>
<style>
:root{--forest:#071711;--forest2:#0d261c;--mint:#dff5e5;--sage:#9ac8a9;--moss:#4b8767;--line:rgba(198,231,208,.18)}
*{box-sizing:border-box}
html,body{height:100%;margin:0}
body{font-family:Inter,Segoe UI,system-ui,sans-serif;color:#f1f7f3;overflow:hidden;background:radial-gradient(circle at 50% 110%,rgba(104,157,121,.35),transparent 42%),linear-gradient(160deg,#06130f 0%,#0a2018 50%,#071711 100%)}
body:before,body:after{content:"";position:absolute;left:-5%;right:-5%;bottom:-8%;height:43%;background:#0a1e17;clip-path:polygon(0 78%,13% 48%,25% 67%,40% 29%,53% 62%,67% 35%,80% 57%,91% 31%,100% 63%,100% 100%,0 100%);opacity:.78}
body:after{bottom:-14%;height:34%;background:#06130f;clip-path:polygon(0 60%,14% 34%,30% 57%,45% 22%,59% 51%,73% 28%,88% 55%,100% 37%,100% 100%,0 100%);opacity:.92}
.wrap{position:relative;z-index:2;height:100%;display:flex;align-items:center;justify-content:center;flex-direction:column;padding:28px}.logo{width:122px;height:122px;filter:drop-shadow(0 14px 30px rgba(0,0,0,.28))}.word{font-size:44px;letter-spacing:.24em;margin:18px 0 5px;font-weight:600;padding-left:.24em}.tag{letter-spacing:.38em;text-transform:uppercase;color:#b8d1c0;font-size:12px;margin-bottom:38px}form{width:min(620px,80vw);position:relative}input{width:100%;border:1px solid rgba(170,211,184,.36);border-radius:24px;background:rgba(4,18,13,.48);color:white;font-size:16px;padding:15px 52px 15px 22px;outline:none;box-shadow:0 15px 45px rgba(0,0,0,.17)}input:focus{border-color:#a9d7b7;box-shadow:0 0 0 3px rgba(145,194,160,.10)}button{position:absolute;right:7px;top:7px;width:40px;height:40px;border:0;border-radius:50%;background:#315f49;color:#eef8f1;font-size:20px;cursor:pointer}.traits{display:flex;gap:28px;margin-top:31px;font-size:10px;letter-spacing:.3em;color:#9fbea9}.traits span+span:before{content:"•";margin-right:28px;color:#76a98a}.note{position:absolute;bottom:24px;letter-spacing:.16em;font-size:10px;color:#6f9580}
</style>
</head>
<body>
<div class="wrap">
<svg class="logo" viewBox="0 0 256 256" role="img" aria-label="Knogn"><defs><linearGradient id="r" x1="0" y1="0" x2="1" y2="1"><stop stop-color="#e6f7ea"/><stop offset=".48" stop-color="#9ac8a9"/><stop offset="1" stop-color="#39785a"/></linearGradient><linearGradient id="l1" x1="0" y1="0" x2="1" y2="1"><stop stop-color="#effaf1"/><stop offset=".55" stop-color="#a2ceb0"/><stop offset="1" stop-color="#4c8b69"/></linearGradient><linearGradient id="l2" x1="0" y1="0" x2="1" y2="1"><stop stop-color="#d1ecd8"/><stop offset=".55" stop-color="#7eb493"/><stop offset="1" stop-color="#2f6b50"/></linearGradient></defs><circle cx="128" cy="128" r="105" fill="none" stroke="url(#r)" stroke-width="12"/><path d="M86 171C73 120 99 78 170 43c19-9 31-18 42-29-8 61-40 102-96 127-17 8-27 18-30 30Z" fill="url(#l1)"/><path d="M92 177c29-37 68-37 105-7 15 12 27 19 43 18-33 31-72 39-108 17-17-11-29-22-40-28Z" fill="url(#l2)"/></svg>
<div class="word">KNOGN</div><div class="tag">A more mindful web</div><form action="https://duckduckgo.com/" method="get"><input autofocus name="q" autocomplete="off" placeholder="Explore. Privately."><button aria-label="Search">→</button></form><div class="traits"><span>PRIVATE</span><span>SECURE</span><span>YOURS</span></div><div class="note">Privacy by design · local by default</div>
</div>
</body>
</html>
)HTML");
}

}  // namespace Branding
