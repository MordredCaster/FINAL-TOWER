#pragma once

namespace FinalTowerRules
{
// The title screen owns the only path to starting a session. Disk writes happen
// after a successful request, never when opening or cancelling the dialog.
class MainMenu
{
public:
    enum class Screen { Home, ConfirmOverwrite, Starting, Playing };
    Screen Current() const { return Page; }
    bool IsOpen() const { return Page != Screen::Playing; }

    bool Continue(bool CanLoad)
    {
        if (Page != Screen::Home || !CanLoad) return false;
        Page = Screen::Starting;
        return true;
    }

    bool NewGame(bool SaveExists)
    {
        if (Page != Screen::Home) return false;
        Page = SaveExists ? Screen::ConfirmOverwrite : Screen::Starting;
        return !SaveExists;
    }

    bool Confirm()
    {
        if (Page != Screen::ConfirmOverwrite) return false;
        Page = Screen::Starting;
        return true;
    }

    void Cancel()
    {
        if (Page == Screen::ConfirmOverwrite) Page = Screen::Home;
    }

    void FinishStart(bool Succeeded)
    {
        if (Page == Screen::Starting) Page = Succeeded ? Screen::Playing : Screen::Home;
    }

private:
    Screen Page = Screen::Home;
};
}
