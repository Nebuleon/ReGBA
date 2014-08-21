#ifndef __REGBA_GUI_H__
#define __REGBA_GUI_H__

#ifdef GIT_VERSION
#  define STRINGIFY(x) XSTRINGIFY(x)
#  define XSTRINGIFY(x) #x
#  define GIT_VERSION_STRING STRINGIFY(GIT_VERSION)
#endif

#define REGBA_VERSION_STRING "1.45.5"

enum MenuEntryKind {
	KIND_OPTION,
	KIND_SUBMENU,
	KIND_DISPLAY,
	KIND_CUSTOM,
};

enum MenuDataType {
	TYPE_STRING,
	TYPE_INT32,
	TYPE_UINT32,
	TYPE_INT64,
	TYPE_UINT64,
};

struct MenuEntry;

struct Menu;

/*
 * MenuModifyFunction is the type of a function that acts on an input in the
 * menu. The function is assigned this input via the MenuEntry struct's
 * various Button<Name>Function members, InitFunction, EndFunction, etc.
 * Variables:
 *   1: On entry into the function, points to a memory location containing
 *     a pointer to the active menu. On exit from the function, the menu may
 *     be modified to a new one, in which case the function has chosen to
 *     activate that new menu; the EndFunction of the old menu is called,
 *     then the InitFunction of the new menu is called.
 * 
 *     The exception to this rule is the NULL menu. If NULL is chosen to be
 *     activated, then no InitFunction is called; additionally, the menu is
 *     exited.
 *   2: On entry into the function, points to a memory location containing the
 *     index among the active menu's Entries array corresponding to the active
 *     menu entry. On exit from the function, the menu entry index may be
 *     modified to a new one, in which case the function has chosen to
 *     activate that new menu entry. If the menu itself has changed, the code
 *     in ReGBA_Menu will activate the first item of that menu.
 */
typedef void (*MenuModifyFunction) (struct Menu**, uint32_t*);

/*
 * MenuEntryDisplayFunction is the type of a function that displays an element
 * (the name or the value, depending on which member receives a function of
 * this type) of a single menu entry.
 * Input:
 *   1: A pointer to the data for the menu entry whose part is being drawn.
 *   2: A pointer to the data for the active menu entry.
 *   3: The position, expressed as a line number starting at 0, of the entry
 *     part to be drawn.
 */
typedef void (*MenuEntryDisplayFunction) (struct MenuEntry*, struct MenuEntry*, uint32_t);

/*
 * MenuEntryCanFocusFunction is the type of a function that determines whether
 * a menu entry can receive the focus.
 * Input:
 *   1: The menu containing the entry that is being tested.
 *   2: The menu entry that is being tested.
 */
typedef bool (*MenuEntryCanFocusFunction) (struct Menu*, struct MenuEntry*);

/*
 * MenuEntryFunction is the type of a function that displays all data related
 * to a menu or that modifies the Target variable of the active menu entry.
 * Input:
 *   1: The menu containing the entries that are being drawn, or the entry
 *     whose Target variable is being modified.
 *   2: The active menu entry.
 */
typedef void (*MenuEntryFunction) (struct Menu*, struct MenuEntry*);

/*
 * MenuInitFunction is the type of a function that runs when a menu is being
 * initialised.
 * Variables:
 *   1: A pointer to a variable holding the menu that is being initialised.
 *   On exit from the function, the menu may be modified to a new one, in
 *   which case the function has chosen to activate that new menu. This can
 *   be used when the initialisation has failed for some reason.
 */
typedef void (*MenuInitFunction) (struct Menu**);

/*
 * MenuFunction is the type of a function that runs when a menu is being
 * finalised or drawn.
 * Input:
 *   1: The menu that is being finalised or drawn.
 */
typedef void (*MenuFunction) (struct Menu*);

/*
 * MenuPersistenceFunction is the type of a function that runs to load or save
 * the value of a persistent setting in a file.
 * Input:
 *   1: The menu entry whose setting's value is being loaded or saved.
 *   2: The text value read from the configuration file, or a buffer with 256
 *   entries to write the configuration name and value to (setting = value).
 */
typedef void (*MenuPersistenceFunction) (struct MenuEntry*, char*);

struct MenuEntry {
	enum MenuEntryKind Kind;
	char* Name;
	char* PersistentName;
	enum MenuDataType DisplayType;
	void* Target;           // With KIND_OPTION, must point to uint32_t.
	                        // With KIND_DISPLAY, must point to the data type
	                        // specified by DisplayType.
	                        // With KIND_SUBMENU, this is struct Menu*.
	// DisplayChoices and ChoiceCount are only used with KIND_OPTION.
	uint32_t ChoiceCount;
	MenuModifyFunction ButtonEnterFunction;
	MenuEntryFunction ButtonLeftFunction;
	MenuEntryFunction ButtonRightFunction;
	MenuEntryDisplayFunction DisplayNameFunction;
	MenuEntryDisplayFunction DisplayValueFunction;
	MenuPersistenceFunction LoadFunction;
	MenuPersistenceFunction SaveFunction;
	MenuEntryCanFocusFunction CanFocusFunction;
	void* UserData;
	struct {
		char* Pretty;
		char* Persistent;
	} Choices[];
};

struct Menu {
	struct Menu* Parent;
	char* Title;
	// The alternate version of a menu is the per-game version of the menu
	// if the menu itself is the global version of it, and vice versa.
	// The alternation is made with GUI_ACTION_ALTERNATE (Select).
	struct Menu* AlternateVersion;
	MenuFunction DisplayBackgroundFunction;
	MenuFunction DisplayTitleFunction;
	MenuEntryFunction DisplayDataFunction;
	MenuModifyFunction ButtonUpFunction;
	MenuModifyFunction ButtonDownFunction;
	MenuModifyFunction ButtonLeaveFunction;
	MenuInitFunction InitFunction;
	MenuFunction EndFunction;
	void* UserData;
	uint32_t ActiveEntryIndex;
	struct MenuEntry* Entries[]; // Entries are ended by a NULL pointer value.
};

extern struct Menu MainMenu;

extern void DefaultLoadFunction(struct MenuEntry* ActiveMenuEntry, char* Value);
extern void DefaultSaveFunction(struct MenuEntry* ActiveMenuEntry, char* Value);

void ShowErrorScreen(const char* Format, ...);

#endif // !defined __REGBA_GUI_H__