#pragma once

#include "../../../game/game.hpp"

bool toggle(const char *const label, bool *const var);
bool selectSingle(const char *const label, int *const var, std::vector<std::pair<std::string, int>> vars);
bool selectMulti(const char *const label, const std::vector<std::pair<std::string, bool *>> vars);
bool sliderInt(const char *const label, int *const var, int min, int max, const char *const fmt = "%d");
bool sliderFloat(const char *const label, float *const var, float min, float max, const char *const fmt = "%.3f");
bool colorPicker(const char *const label, Color *const var);
bool inputString(const char *const label, std::string *const var, const char *const fmt = "%s", int flags = 0);
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
