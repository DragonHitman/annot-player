// messageview.cc
// 10/16/2011

#include "messageview.h"
#include "messagehandler.h"
#include "checkboxgrid.h"
#include "radiobuttongrid.h"
#include "textcodecmanager.h"
#include "tr.h"
#include "global.h"
#ifdef WITH_WIN_TEXTHOOK
# include "win/texthook/texthook.h"
#endif // WITH_WIN_TEXTHOOK
#include "project/common/acui.h"
#include "module/qtext/htmltag.h"
#include "module/qtext/spinbox.h"
#include <QtGui>

//#define DEBUG "messageview"
#include "module/debug/debug.h"

#define WINDOW_FLAGS ( \
  Qt::Dialog | \
  Qt::CustomizeWindowHint | \
  Qt::WindowTitleHint | \
  Qt::WindowSystemMenuHint | \
  Qt::WindowMinMaxButtonsHint | \
  Qt::WindowCloseButtonHint )

#define HOOKCOMBOBOX_MINWIDTH       40
#define HOOKCOMBOBOX_MAXWIDTH       60

// - Construction -

MessageView::MessageView(MessageHandler *h, QWidget *parent)
  : Base(parent, WINDOW_FLAGS), active_(false), messageHandler_(h)
{
  setWindowTitle(tr("Message view"));

  createLayout();

  setActive(true); // always active
  encodingEdit_->setFocus();
}

void
MessageView::createLayout()
{
  AcUi *ui = AcUi::globalInstance();
  ui->setWindowStyle(this);

  enum { MaxThreadCount = 50, ThreadGridColumn = 4 };
#ifdef WITH_WIN_TEXTHOOK
  //static_assert(MaxThreadCount >= TextHook::capacity(), "thread capacity");
  Q_ASSERT(MaxThreadCount >= TextHook::capacity());
#endif // WITH_WIN_TEXTHOOK

  leadingThreads_ = new RadioButtonGrid(MaxThreadCount, ThreadGridColumn, this);
  supportThreads_ = new CheckBoxGrid(MaxThreadCount, ThreadGridColumn, this);

  connect(leadingThreads_, SIGNAL(currentIndexChanged(int)), SLOT(updateGrid()));
  connect(leadingThreads_, SIGNAL(currentIndexChanged(int)), SLOT(updateButtons()));
  connect(leadingThreads_, SIGNAL(currentIndexChanged(int)), SLOT(updateText()));
  connect(supportThreads_, SIGNAL(selectionChanged()), SLOT(updateButtons()));
  connect(supportThreads_, SIGNAL(selectionChanged()), SLOT(updateText()));

  textEdit_ = ui->makeTextEdit(AcUi::ReadOnlyHint, tr("Game messages")); {
    //QTextCharFormat fmt;
    //fmt.setBackground(Qt::red);
    //QTextCursor tc = textEdit_->textCursor();
    //tc.mergeCharFormat(fmt);
    //textEdit_->setTextCursor(tc);
  }
  //connect(textEdit_, SIGNAL(cursorPositionChanged()), SLOT(invalidateCurrentCharFormat()));

  QString ja = tr("ja"),
          en = tr("en"),
          chs = tr("chs"),
          cht = tr("cht"),
          ko = tr("ko");
  QStringList defaultEncodings = QStringList()
    << QString("SHIFT-JIS [%1]").arg(ja)
    << "UTF-16"
    //<< "UTF-8"
    //<< QString("EUC-JP [%1]").arg(ja)
    //<< QString("ISO-2022-JP [%1]").arg(ja)
    //<< QString("ISO-8859-1 [%1]").arg(en)
    << QString("BIG5 [%1]").arg(cht)
    << QString("GBK [%1]").arg(chs)
    << QString("EUC-KR [%1]").arg(ko)
    //<< "UNICODE"
    << TEXT_CODEC_DEFAULT;

  encodingEdit_ = ui->makeComboBox(AcUi::ReadOnlyHint, "", tr("Text Encoding"), tr("Encoding"), defaultEncodings);
  connect(encodingEdit_, SIGNAL(activated(QString)), SLOT(setEncoding(QString)));

  //hookIndexEdit_ = new QtExt::SpinBox; {
  //  hookIndexEdit_->setToolTip(tr("Signal channel"));
  //  //hookIndexEdit_->setMinimumWidth(HOOKCOMBOBOX_MINWIDTH);
  //  //hookIndexEdit_->setMaximumWidth(HOOKCOMBOBOX_MAXWIDTH);
  //  //if (hookIndexEdit_->isEditable())
  //  //  hookIndexEdit_->lineEdit()->setAlignment(Qt::AlignRight);
  //  hookIndexEdit_->setMaximum(0);
  //  hookIndexEdit_->setMinimum(0);
  //  hookIndexEdit_->setEnabled(false);
  //}

  //autoButton_ = ui->makeToolButton(
  //      AcUi::CheckHint, TR(T_AUTO), tr("Auto-detect signal"), this, SLOT(invalidateCurrentHook()));
  //autoButton_->setChecked(false);

  selectButton_ = ui->makeToolButton(
        AcUi::PushHint | AcUi::HighlightHint | AcUi::InvertHint, TR(T_OK), tr("Listen to selected channel"), this, SLOT(select()));

  resetButton_ = ui->makeToolButton(
        AcUi::PushHint, TR(T_RESET), tr("Reset changes and texts"), this, SLOT(reset()));

  //hookCountLabel_ = ui->makeLabel(0, "/0", tr("Current signal"), hookIndexEdit_);

  // Set layout

  QGroupBox *leadingGroup = ui->makeGroupBox(AcUi::TabHint, tr("Leading Thread")),
            *supportGroup = ui->makeGroupBox(AcUi::TabHint, tr("Supplementary Threads"));

  leadingGroup->setLayout(leadingThreads_->layout());
  supportGroup->setLayout(supportThreads_->layout());

  QVBoxLayout *rows = new QVBoxLayout; {
    QHBoxLayout *header = new QHBoxLayout;
    rows->addLayout(header);
    rows->addWidget(leadingGroup);
    rows->addWidget(supportGroup);
    rows->addWidget(textEdit_);

    header->addWidget(encodingEdit_);
    //header->addWidget(hookIndexEdit_);
    //header->addWidget(hookCountLabel_);
    //header->addWidget(autoButton_); // TODO: auto detect is disabled, because hookName is unimplemented
    //header->addStretch();
    header->addWidget(resetButton_);
    header->addWidget(selectButton_);

    // left, top, right, bottom
    header->setContentsMargins(0, 0, 0, 0);
    rows->setContentsMargins(4, 2, 4, 4);
  } setLayout(rows);
}

// - Properties -

void
MessageView::setActive(bool active)
{
  active_ = active;
#ifdef WITH_WIN_TEXTHOOK
  if (active_) {
    connect(TextHook::globalInstance(), SIGNAL(messageReceived(QByteArray,qint64,QString)),
            SLOT(processMessage(QByteArray,qint64,QString)));
    connect(TextCodecManager::globalInstance(), SIGNAL(encodingChanged(QString)),
            SLOT(refresh()));
  } else {
    disconnect(TextHook::globalInstance(), SIGNAL(messageReceived(QByteArray,qint64,QString)),
               this, SLOT(processMessage(QByteArray,qint64,QString)));
    disconnect(TextCodecManager::globalInstance(), SIGNAL(encodingChanged(QString)),
               this, SLOT(refresh()));
  }
#endif // WITH_WIN_TEXTHOOK
}

void
MessageView::refresh()
{
  //updateLeadingGrid();
  //updateSupportingGrid();
  updateGrid();
  updateButtons();
  updateText();
  refreshEncodingEdit();
}

void
MessageView::refreshEncodingEdit()
{
  int i = encodingEdit_->findText(TextCodecManager::globalInstance()->encoding());
  if (i >= 0 && i < encodingEdit_->count())
    encodingEdit_->setCurrentIndex(i);
}

void
MessageView::setVisible(bool visible)
{
  //if (active_ != visible)
  //  setActive(visible);

  if (visible)
    refresh();
  Base::setVisible(visible);
}

// - Actions -

void
MessageView::addMessages(const QList<QByteArray> &l, qint64 signature, const QString &provider)
{
  foreach (const QByteArray &data, l)
    processMessage(data, signature, provider);
}

void
MessageView::select()
{
  if (!leadingThreads_->hasSelection())
    return;

  TextThread t;
  QString provider = leadingThreads_->currentText();
  t.setRole(TextThread::LeadingRole);
  t.setProvider(provider);
  t.setSignature(signatures_[leadingThreads_->currentIndex()]);

  TextThreadList l;
  l.append(t);

  emit message(tr("subscribe to main thread") + ": " + provider);

  if (supportThreads_->hasSelection()) {
    t.setRole(TextThread::SupportRole);
    foreach (int index,  supportThreads_->currentIndices()) {
      provider = leadingThreads_->itemText(index);
      t.setSignature(signatures_[index]);
      t.setProvider(provider);
      l.append(t);
      emit message(tr("subscribe to thread") + ": " HTML_SS(+provider+, color:orange));
    }
  }
  emit threadsSelected(l);
}

void
MessageView::setThreads(const TextThreadList &l)
{
  clear();
  int index = 0;
  foreach (const TextThread &t, l) {
    signatures_.append(t.signature());
    leadingThreads_->addItem(t.provider());

    supportThreads_->addItem(
          QString::number(supportThreads_->size() +1) + ":" +
          t.provider());

    switch (t.role()) {
    case TextThread::LeadingRole: leadingThreads_->setCurrentIndex(index); break;
    case TextThread::SupportRole: supportThreads_->setItemChecked(index, true); break;
    default: Q_ASSERT(0);
    }
    index++;
  }
}

void
MessageView::reset()
{
  if (messageHandler_->hasThreads())
    setThreads(messageHandler_->threads());
  else
    clear();
}

void
MessageView::clear()
{
  supportThreads_->clear();
  leadingThreads_->clear();
  signatures_.clear();

  messages_.clear();

  textEdit_->clear();

  //invalidateHookCountLabel();
  updateButtons();

  emit message(tr("messages cleared"));
}

//void
//MessageView::invalidateHookCountLabel()
//{
//  int count = signatures_.size() - 1;
//  hookCountLabel_->setText(QString("/%1 ").arg(QString::number(count)));
//}

void
MessageView::updateGrid()
{
  supportThreads_->setItemsEnabled(true);
  int index = leadingThreads_->currentIndex();
  if (index >= 0)
    supportThreads_->setItemEnabled(index, false);
}

void
MessageView::updateButtons()
{
  //if (supportThreads_->itemCount() > 1 && supportThreads_->contains(0))
  //  supportThreads_->setItemChecked(0, false);

  selectButton_->setEnabled(leadingThreads_->hasSelection());
  //resetButton_->setEnabled(!isEmpty());
}

void
MessageView::updateText()
{
  if (isEmpty()) {
    textEdit_->clear();
    return;
  }

  QString html;
  bool leadingThreadsSelected = leadingThreads_->hasSelection(),
       supportThreadsSelected = supportThreads_->hasSelection();
  if (!leadingThreadsSelected && !supportThreadsSelected) {
    int line = 0;
    foreach (const TextMessage &m, messages_) {
      QString t = TextCodecManager::globalInstance()->decode(m.data());
      html.append(line++ % 2 ?
        HTML_BR() HTML_SS(+t+, color:purple) HTML_BR() :
        HTML_BR() HTML_SS(+t+, color:blue) HTML_BR());
    }
  } else if (leadingThreadsSelected) {
    qint64 leadingSignature = signatures_[leadingThreads_->currentIndex()];
    if (!supportThreadsSelected) {
      foreach (const TextMessage &m, messages_)
        if (m.signature() == leadingSignature) {
          QString t = TextCodecManager::globalInstance()->decode(m.data());
          html.append(HTML_BR() HTML_SS(+t+, color:red) HTML_BR());
        }
    } else {
      auto l = supportThreads_->currentIndices();
      foreach (const TextMessage &m, messages_)
        if (m.signature() == leadingSignature) {
          QString t = TextCodecManager::globalInstance()->decode(m.data());
          html.append(HTML_BR() HTML_SS(+t+, color:red) HTML_BR());
        } else
          foreach (int index, l)
            if (m.signature() == signatures_[index]) {
              QString t = TextCodecManager::globalInstance()->decode(m.data());
              html.append(index % 2 ?
                HTML_BR() +QString::number(index+1)+ ": " HTML_SS(+t+, color:purple) HTML_BR() :
                HTML_BR() +QString::number(index+1)+ ": " HTML_SS(+t+, color:blue) HTML_BR());
            }

    }
  }
  textEdit_->setHtml(html);
  textEdit_->moveCursor(QTextCursor::End);
  //textEdit_->ensureCursorVisible();

  //QTextCursor tc = textEdit_->textCursor();
  //tc.movePosition(QTextCursor::End);
  //textEdit_->setTextCursor(tc);
}

//bool
//MessageView::isBetterHook(ulong goodHookId, ulong badHookId)
//{
//  QString badHookName = TextHook::globalInstance()->hookNameById(badHookId);
//  if (badHookName.isEmpty())
//    return true;
//  QString goodHookName = TextHook::globalInstance()->hookNameById(goodHookId);
//  if (goodHookName.isEmpty())
//    return false;
//
//  return TextHook::globalInstance()->isStandardHookName(badHookName) ||
//         !TextHook::globalInstance()->isStandardHookName(goodHookName);
//}

void
MessageView::processMessage(const QByteArray &data, qint64 signature, const QString &provider)
{
  DOUT("enter: signature =" << signature << ", data size =" << data.size());

  if (data.isEmpty() || !signature) {
    DOUT("exit: skip empty message");
    return;
  }

  messages_.append(TextMessage(data, signature));

  //if (isEmpty()) {
  //  supportThreads_->addItem(tr("All"));
  //  //supportThreads_->setItemChecked(0, true);
  //}

  if (!signatures_.contains(signature)) {
    signatures_.append(signature);
    leadingThreads_->addItem(provider);

    supportThreads_->addItem(
          QString::number(supportThreads_->size() +1) + ":" +
          provider);

    emit message(tr("new text thread discovered") + ": " HTML_SS(+provider+, color:orange));
  }

  updateText();

  DOUT("exit");
}

//void
//MessageView::invalidateCurrentHook()
//{
//  if (processName_.isEmpty())
//    return;
//
//  ulong hid = currentHookId();
//  QString hookName = TextHook::globalInstance()->hookNameById(hid);
//  if (!hookName.isEmpty() && TextHook::globalInstance()->isKnownHookForProcess(hookName, processName_))
//    selectCurrentHook();
//}

void
MessageView::invalidateCurrentCharFormat()
{
  QTextCharFormat fmt;
  fmt.setForeground(Qt::red);
  textEdit_->mergeCurrentCharFormat(fmt);
}

void
MessageView::setEncoding(const QString &name)
{
  QString e = name;
  e.remove(QRegExp(" .*"));
  DOUT("encoding =" << e);
  TextCodecManager::globalInstance()->setEncoding(e);
}

// - Events -

//void MessageView::dragEnterEvent(QDragEnterEvent *event)     { emit dragEnterEventReceived(event); }
//void MessageView::dragMoveEvent(QDragMoveEvent *event)       { emit dragMoveEventReceived(event); }
//void MessageView::dragLeaveEvent(QDragLeaveEvent *event)     { emit dragLeaveEventReceived(event); }
//void MessageView::dropEvent(QDropEvent *event)               { emit dropEventReceived(event); }

// EOF
