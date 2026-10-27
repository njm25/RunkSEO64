#pragma once

#include "../../../game/game.hpp"

struct Bind;
enum class BindType;

enum class WidgetMode
{
	DRAW,     //normal menu
	REGISTER, //walk the pages without drawing, filling the section registry
	SEARCH    //draw only the sections and widgets matching the search query
};

bool toggle(const char *const label, bool *const var);
bool selectSingle(const char *const label, int *const var, std::vector<std::pair<std::string, int>> vars);
bool selectMulti(const char *const label, const std::vector<std::pair<std::string, bool *>> vars);
bool sliderInt(const char *const label, int *const var, int min, int max, const char *const fmt = "%d");
bool sliderFloat(const char *const label, float *const var, float min, float max, const char *const fmt = "%.3f");
bool colorPicker(const char *const label, Color *const var);
bool inputString(const char *const label, std::string *const var, const char *const fmt = "%s", int flags = 0);
bool inputKey(int *const var, const bool start_listening = false);
bool inputStringWithHint(const char *const label, std::string *const var, const char *const hint, const char *const fmt = "%s", int flags = 0);

//layout
void beginPage(const char *const id, const float bottom_padding);
void endPage();
void beginColumn(const char *const id);
void endColumn();
void nextColumn();
void section(const char *const id, const char *const title, const float width, const std::function<void()> &body, const float item_width = 100.0f);
void sep();
void heading(const char *const text);

//modal yes/cancel prompt, drawn by drawConfirm() once per frame from the menu window
void confirm(std::string_view msg, const std::function<void()> &on_confirm);
void drawConfirm();

//search
void setWidgetMode(const WidgetMode new_mode);
WidgetMode widgetMode();
void setSearchQuery(std::string_view query);
void setSearchPath(std::string_view path);
bool searchGotoClicked();
int searchResultCount();

//binds
ImVec4 warningColor();
bool editBindType(const char *const id, BindType *const type);
bool editBindValue(const char *const id, const Bind &bind, std::any *const val);
std::string bindKeyUsers(const int key, const Bind *const ignore); //names of the binds using a key, comma separated
