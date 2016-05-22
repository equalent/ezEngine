#include <GuiFoundation/PCH.h>
#include <GuiFoundation/PropertyGrid/Implementation/ManipulatorLabel.moc.h>
#include <qevent.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <QFont>
#include <GuiFoundation/UIServices/UIServices.moc.h>

ezManipulatorLabel::ezManipulatorLabel(QWidget* parent, Qt::WindowFlags f)
  : QLabel(parent, f), m_pItems(nullptr), m_pManipulator(nullptr), m_bActive(false)
{

}

ezManipulatorLabel::ezManipulatorLabel(const QString& text, QWidget* parent, Qt::WindowFlags f)
  : QLabel(text, parent, f), m_pItems(nullptr), m_pManipulator(nullptr), m_bActive(false)
{
}

const ezManipulatorAttribute* ezManipulatorLabel::GetManipulator() const
{
  return m_pManipulator;
}

void ezManipulatorLabel::SetManipulator(const ezManipulatorAttribute* pManipulator)
{
  m_pManipulator = pManipulator;

  if (m_pManipulator)
  {
    setCursor(Qt::PointingHandCursor);
    setForegroundRole(QPalette::ColorRole::Link);
  }
}

const bool ezManipulatorLabel::GetManipulatorActive() const
{
  return m_bActive;
}

void ezManipulatorLabel::SetManipulatorActive(bool bActive)
{
  m_bActive = bActive;

  if (m_pManipulator)
  {
    QFont f = font();
    f.setBold(m_bActive);
    setFont(f);
    setForegroundRole(m_bActive ? QPalette::ColorRole::LinkVisited : QPalette::ColorRole::Link);
  }
}

void ezManipulatorLabel::SetSelection(const ezHybridArray<ezQtPropertyWidget::Selection, 8>& items)
{
  m_pItems = &items;
}

void ezManipulatorLabel::mousePressEvent(QMouseEvent *ev)
{
  if (ev->button() != Qt::LeftButton)
    return;

  if (m_pManipulator == nullptr)
    return;

  const ezDocument* pDoc = (*m_pItems)[0].m_pObject->GetDocumentObjectManager()->GetDocument();

  if (m_bActive)
    ezManipulatorManager::GetSingleton()->ClearActiveManipulator(pDoc);
  else
    ezManipulatorManager::GetSingleton()->SetActiveManipulator(pDoc, m_pManipulator, *m_pItems);
}

void ezManipulatorLabel::enterEvent(QEvent* ev)
{
  if (m_pManipulator)
  {
    QFont f = font();
    f.setUnderline(true);
    f.setBold(m_bActive);
    setFont(f);
  }

  QLabel::enterEvent(ev);
}

void ezManipulatorLabel::leaveEvent(QEvent* ev)
{
  if (m_pManipulator)
  {
    QFont f = font();
    f.setUnderline(false);
    f.setBold(m_bActive);
    setFont(f);
  }

  QLabel::leaveEvent(ev);
}