#include "FTMainMenuRules.h"
#include <cassert>
#include <iostream>

using FinalTowerRules::MainMenu;
using Screen = MainMenu::Screen;

int main()
{
    // No save: Continue cannot start gameplay; a new adventure needs no prompt.
    MainMenu Fresh;
    assert(Fresh.IsOpen() && Fresh.Current() == Screen::Home);
    assert(!Fresh.Continue(false) && !Fresh.Confirm());
    Fresh.FinishStart(true); // An unsolicited/stale completion must not dismiss the menu.
    assert(Fresh.Current() == Screen::Home);
    assert(Fresh.NewGame(false));
    assert(Fresh.Current() == Screen::Starting && Fresh.IsOpen());
    assert(!Fresh.NewGame(false) && !Fresh.Continue(true) && !Fresh.Confirm());
    Fresh.FinishStart(true);
    assert(!Fresh.IsOpen());
    assert(!Fresh.NewGame(true) && !Fresh.Continue(true) && !Fresh.Confirm());
    Fresh.Cancel();
    assert(Fresh.Current() == Screen::Playing);

    // An existing slot (valid OR corrupt) can only be replaced via confirmation.
    for (bool Loadable : {false, true})
    {
        MainMenu Existing;
        int Writes = 0;
        if (Existing.NewGame(true)) ++Writes;
        assert(Writes == 0 && Existing.Current() == Screen::ConfirmOverwrite);
        assert(!Existing.NewGame(false) && !Existing.Continue(Loadable));
        Existing.Cancel();
        assert(Writes == 0 && Existing.Current() == Screen::Home);
        assert(!Existing.Confirm()); // A stale confirmation after cancellation is harmless.
        assert(!Existing.NewGame(true));
        if (Existing.Confirm()) ++Writes;
        if (Existing.Confirm()) ++Writes; // Double tap cannot repeat the write.
        assert(Writes == 1);
        Existing.FinishStart(false); // Disk full: stay at title and require a new confirmation.
        assert(Existing.IsOpen() && Existing.Current() == Screen::Home);
        assert(!Existing.Confirm());
        assert(!Existing.NewGame(true));
        assert(Existing.Confirm());
        Existing.FinishStart(true);
        assert(!Existing.IsOpen());
    }

    // A failed read (e.g. a removed save) never enters gameplay.
    MainMenu Resume;
    assert(Resume.Continue(true));
    Resume.FinishStart(false);
    assert(Resume.Current() == Screen::Home);
    assert(!Resume.Continue(false));
    assert(Resume.Continue(true));
    Resume.FinishStart(true);
    assert(Resume.Current() == Screen::Playing);

    // A save appearing after opening the menu still requires confirmation.
    MainMenu ChangedDisk;
    assert(!ChangedDisk.NewGame(true));
    assert(ChangedDisk.Current() == Screen::ConfirmOverwrite);
    std::cout << "Main menu: new game, resume, cancellation, overwrite consent, duplicate inputs and I/O failures passed\n";
}
