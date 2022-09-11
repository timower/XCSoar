#include "PlaneDialogs.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Plane/Plane.hpp"
#include "Widget/TextListWidget.hpp"
#include "Form/DataField/Listener.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "util/ConvertString.hpp"


class FlapSettingWidget final
  : public RowFormWidget, DataFieldListener {

  enum Controls {
    NAME,
    SPEED
  };

    FlapSetting value;

public:
  FlapSettingWidget(const DialogLook &_look)
    :RowFormWidget(_look) {}

  const FlapSetting &GetValue() const {
    return value;
  }

  void SetValue(FlapSetting setting);

private:

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

class FlapListWidget final
  : public TextListWidget/*,
    public PageLayoutEditWidget::Listener*/ {

  Plane plane;
  FlapSettingWidget* editor;

public:
  FlapListWidget(Plane _plane) : plane(_plane) {}


  void SetEditor(FlapSettingWidget& _editor) {
    editor = &_editor;
  }

  const Plane& GetValue() const noexcept {
    return plane;
  }

  void CreateButtons(WidgetDialog& dialog) noexcept;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;

  void Show(const PixelRect& rc) noexcept override;

protected:
  /* virtual methods from TextListWidget */
  const TCHAR *GetRowText(unsigned i) const noexcept override ;

  /* virtual methods from class ListCursorHandler */
  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override {
    return true;
  }

  void OnCursorMoved(unsigned index) noexcept override;
  void OnActivateItem([[maybe_unused]] unsigned index) noexcept override;
private:
  void UpdateList();
};

void
FlapSettingWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                          [[maybe_unused]] const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);
  AddText(_("Name"), nullptr, value.name, this);
  AddFloat(_("Min. Speed"), nullptr,
           _T("%.0f %s"), _T("%.0f"), 0, 300, 5,
           false, UnitGroup::HORIZONTAL_SPEED, value.minV);
}

void
FlapSettingWidget::OnModified([[maybe_unused]] DataField &df) noexcept
{
}

void
FlapSettingWidget::SetValue(FlapSetting setting)
{
  value = setting;
  LoadValue(Controls::NAME, value.name);
  LoadValue(Controls::SPEED, value.minV, UnitGroup::HORIZONTAL_SPEED);
}

void FlapListWidget::UpdateList() {
  ListControl &list_control = GetList();
  list_control.SetLength(plane.polar_shape.flaps.size());
  list_control.Invalidate();
}

void
FlapListWidget::Show(const PixelRect& rc) noexcept
{
  if (plane.polar_shape.flaps.size() > 0) {
    editor->SetValue(plane.polar_shape.flaps[GetList().GetCursorIndex()]);
  }
  TextListWidget::Show(rc);
}

void
FlapListWidget::Prepare(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
{
  TextListWidget::Prepare(parent, rc);
  UpdateList();
}

void
FlapListWidget::CreateButtons(WidgetDialog& dialog) noexcept
{
  dialog.AddButton(_("+"), []{});
  dialog.AddButton(_("-"), []{});
}

const TCHAR*
FlapListWidget::GetRowText([[maybe_unused]] unsigned i) const noexcept {
  return plane.polar_shape.flaps[i].name;
}


// bool
// FlapListWidget::Save(bool &_changed) noexcept
// {
//   bool changed = false;
//   _changed |= changed;
//   return true;
// }
void
FlapListWidget::OnCursorMoved(unsigned i) noexcept
{
  editor->SetValue(plane.polar_shape.flaps[i]);
}

void
FlapListWidget::OnActivateItem(unsigned i) noexcept
{
  editor->SetValue(plane.polar_shape.flaps[i]);
}

bool
dlgPlaneFlapsShowModal(Plane &_plane)
{
  StaticString<128> caption;
  caption.Format(_T("%s: %s"), _("Plane Flaps"), _plane.registration.c_str());

  const DialogLook &look = UIGlobals::GetDialogLook();

  auto _list = std::make_unique<FlapListWidget>(_plane);
  auto _editor = std::make_unique<FlapSettingWidget>(look);

  TWidgetDialog<TwoWidgets>
    dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(), look, caption);
  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);
  dialog.SetWidget(std::move(_list), std::move(_editor));

  auto& two = (TwoWidgets&)dialog.GetWidget();
  auto &list = (FlapListWidget &)two.GetFirst();
  auto &editor = (FlapSettingWidget &)two.GetSecond();
  list.SetEditor(editor);

  list.CreateButtons(dialog);

  // list.SetEditor(editor);
  // dialog.GetWidget().CreateButtons(dialog);
  //
  const int result = dialog.ShowModal();

  if (result != mrOK)
    return false;

  _plane = list.GetValue();
  return true;
}
