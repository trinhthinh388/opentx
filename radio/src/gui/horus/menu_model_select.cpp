/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "../../opentx.h"

#define CATEGORIES_WIDTH               140
#define MODELCELL_WIDTH                153
#define MODELCELL_HEIGHT               61

enum ModelSelectMode {
  MODE_SELECT_CATEGORY,
  MODE_SELECT_MODEL,
  MODE_RENAME_CATEGORY,
};

uint8_t selectMode;
uint8_t currentCategory;
char selectedFilename[LEN_MODEL_FILENAME+1];
char selectedCategory[LEN_MODEL_FILENAME+1];

void drawCategory(coord_t y, const char * name, bool selected)
{
  if (selected) {
    lcdDrawSolidFilledRect(0, y-INVERT_VERT_MARGIN, CATEGORIES_WIDTH, INVERT_LINE_HEIGHT+2, HEADER_BGCOLOR);
    lcdDrawBitmapPattern(CATEGORIES_WIDTH-12, y, LBM_LIBRARY_CURSOR, MENU_TITLE_COLOR);
    if (selectMode == MODE_SELECT_CATEGORY) {
      drawShadow(0, y-INVERT_VERT_MARGIN, CATEGORIES_WIDTH, INVERT_LINE_HEIGHT+2);
    }
  }
  lcdDrawText(MENUS_MARGIN_LEFT, y, name, MENU_TITLE_COLOR);
}

void drawModel(coord_t x, coord_t y, const char * name, bool selected)
{
  ModelHeader header;
  const char * error = readModel(name, (uint8_t *)&header, sizeof(header));
  if (error) {
    lcdDrawText(x+5, y+2, "(Invalid Model)", TEXT_COLOR);
    lcdDrawBitmapPattern(x+5, y+23, LBM_LIBRARY_SLOT, TEXT_COLOR);
  }
  else {
    lcdDrawSizedText(x+5, y+2, header.name, LEN_MODEL_NAME, ZCHAR|TEXT_COLOR);
    putsTimer(x+104, y+41, 0, TEXT_COLOR|LEFT);
    for (int i=0; i<4; i++) {
      lcdDrawBitmapPattern(x+104+i*11, y+25, LBM_SCORE0, TITLE_BGCOLOR);
    }
    uint8_t * bitmap = bmpLoad(header.bitmap);
    if (bitmap) {
      lcdDrawBitmap(x+5, y+24, bitmap, 0, 0, getBitmapScale(bitmap, 64, 32));
      free(bitmap);
    }
    else {
      lcdDrawBitmapPattern(x+5, y+23, LBM_LIBRARY_SLOT, TEXT_COLOR);
    }
  }
  lcdDrawSolidHorizontalLine(x+5, y+19, 143, LINE_COLOR);
  if (selected) {
    lcdDrawSolidRect(x, y, MODELCELL_WIDTH, MODELCELL_HEIGHT, 1, TITLE_BGCOLOR);
    drawShadow(x, y, MODELCELL_WIDTH, MODELCELL_HEIGHT);
  }
}

void onCategorySelectMenu(const char * result)
{
  if (result == STR_CREATE_MODEL) {
    storageCheck(true);
    createModel(currentCategory);
    selectMode = MODE_SELECT_MODEL;
    menuVerticalPosition = 255;
  }
  else if (result == STR_CREATE_CATEGORY) {
    storageInsertCategory("Category", -1);
  }
  else if (result == STR_RENAME_CATEGORY) {
    selectMode = MODE_RENAME_CATEGORY;
    s_editMode = EDIT_MODIFY_STRING;
    editNameCursorPos = 0;
  }
  else if (result == STR_DELETE_CATEGORY) {
    storageRemoveCategory(currentCategory--);
  }
}

void onModelSelectMenu(const char * result)
{
  if (result == STR_SELECT_MODEL) {
    memcpy(g_eeGeneral.currModelFilename, selectedFilename, LEN_MODEL_FILENAME);
    storageDirty(EE_GENERAL);
    storageCheck(true);
    loadModel(g_eeGeneral.currModelFilename);
    chainMenu(menuMainView);
  }
  else if (result == STR_DELETE_MODEL) {
    POPUP_CONFIRMATION(STR_DELETEMODEL);
    SET_WARNING_INFO(selectedFilename, LEN_MODEL_FILENAME, 0);
  }
  else if (result == STR_DUPLICATE_MODEL) {
    char duplicatedFilename[LEN_MODEL_FILENAME+1];
    memcpy(duplicatedFilename, selectedFilename, sizeof(duplicatedFilename));
    if (findNextFileIndex(duplicatedFilename, MODELS_PATH)) {
      sdCopyFile(selectedFilename, MODELS_PATH, duplicatedFilename, MODELS_PATH);
      storageInsertModel(duplicatedFilename, currentCategory, -1);
    }
    else {
      POPUP_WARNING("Invalid Filename");
    }
  }
}

#define MODEL_INDEX()       (menuVerticalPosition*2+menuHorizontalPosition)

bool menuModelSelect(evt_t event)
{
  if (warningResult) {
    warningResult = 0;
    int modelIndex = MODEL_INDEX();
    storageRemoveModel(currentCategory, modelIndex);
    s_copyMode = 0;
    event = EVT_REFRESH;
    if (modelIndex > 0) {
      modelIndex--;
      menuVerticalPosition = modelIndex / 2;
      menuHorizontalPosition = modelIndex & 1;
    }
  }

  switch(event) {
    case 0:
      // no need to refresh the screen
      return false;

    case EVT_ENTRY:
      selectMode = MODE_SELECT_CATEGORY;
      currentCategory = menuVerticalPosition = 0;
      break;

    case EVT_KEY_FIRST(KEY_EXIT):
      switch (selectMode) {
        case MODE_SELECT_CATEGORY:
          chainMenu(menuMainView);
          return false;
        case MODE_SELECT_MODEL:
          selectMode = MODE_SELECT_CATEGORY;
          menuVerticalPosition = currentCategory;
          break;
      }
      break;

    case EVT_KEY_BREAK(KEY_ENTER):
    case EVT_KEY_BREAK(KEY_RIGHT):
      switch (selectMode) {
        case MODE_SELECT_CATEGORY:
          selectMode = MODE_SELECT_MODEL;
          menuVerticalPosition = 0;
          break;
      }
      break;

    case EVT_KEY_LONG(KEY_ENTER):
      if (selectMode == MODE_SELECT_CATEGORY) {
        killEvents(event);
        POPUP_MENU_ADD_ITEM(STR_CREATE_MODEL);
        POPUP_MENU_ADD_ITEM(STR_CREATE_CATEGORY);
        POPUP_MENU_ADD_ITEM(STR_RENAME_CATEGORY);
        if (currentCategory > 0)
          POPUP_MENU_ADD_ITEM(STR_DELETE_CATEGORY);
        popupMenuHandler = onCategorySelectMenu;
      }
      else if (selectMode == MODE_SELECT_MODEL) {
        killEvents(event);
        ModelHeader header;
        const char * error = readModel(selectedFilename, (uint8_t *)&header, sizeof(header));
        if (!error) {
          POPUP_MENU_ADD_ITEM(STR_SELECT_MODEL);
          POPUP_MENU_ADD_ITEM(STR_DUPLICATE_MODEL);
        }
        // POPUP_MENU_ADD_SD_ITEM(STR_BACKUP_MODEL);
        // POPUP_MENU_ADD_ITEM(STR_MOVE_MODEL);
        POPUP_MENU_ADD_ITEM(STR_DELETE_MODEL);
        // POPUP_MENU_ADD_ITEM(STR_CREATE_MODEL);
        // POPUP_MENU_ADD_ITEM(STR_RESTORE_MODEL);
        popupMenuHandler = onModelSelectMenu;
      }
      break;
  }

  // Header
  theme->drawTopbarBackground(LBM_LIBRARY_ICON);

  // Body
  lcdDrawSolidFilledRect(0, MENU_HEADER_HEIGHT, CATEGORIES_WIDTH, LCD_H-MENU_HEADER_HEIGHT-MENU_FOOTER_HEIGHT, TITLE_BGCOLOR);
  lcdDrawSolidFilledRect(CATEGORIES_WIDTH, MENU_HEADER_HEIGHT, LCD_W-CATEGORIES_WIDTH, LCD_H-MENU_HEADER_HEIGHT-MENU_FOOTER_HEIGHT, TEXT_BGCOLOR);

  // Footer
  lcdDrawSolidFilledRect(0, MENU_FOOTER_TOP, LCD_W, MENU_FOOTER_HEIGHT, HEADER_BGCOLOR);

  // Categories
  StorageModelsList storage;
  const char * error = storageOpenModelsList(&storage);
  if (!error) {
    bool result = true;
    coord_t y = MENU_HEADER_HEIGHT+10;
    int index = 0;
    while (1) {
      char line[LEN_MODEL_FILENAME+1];
      result = storageReadNextCategory(&storage, line, LEN_MODEL_FILENAME);
      if (!result)
        break;
      if (y < LCD_H) {
        if (selectMode == MODE_RENAME_CATEGORY && currentCategory == index) {
          lcdDrawSolidFilledRect(0, y-INVERT_VERT_MARGIN, CATEGORIES_WIDTH, INVERT_LINE_HEIGHT, TEXT_BGCOLOR);
          editName(MENUS_MARGIN_LEFT, y, selectedCategory, LEN_MODEL_FILENAME, event, 1, 0);
          if (s_editMode == 0 || event == EVT_KEY_BREAK(KEY_EXIT)) {
            storageRenameCategory(currentCategory, selectedCategory);
            selectMode = MODE_SELECT_CATEGORY;
            putEvent(EVT_REFRESH);
          }
        }
        else {
          if (currentCategory == index) {
            memset(selectedCategory, 0, sizeof(selectedCategory));
            strncpy(selectedCategory, line, sizeof(selectedCategory));
          }
          drawCategory(y, line, currentCategory==index);
        }
      }
      y += FH;
      index++;
    }
    if (selectMode == MODE_SELECT_CATEGORY) {
      if (navigate(event, index, 9)) {
        TRACE("Refresh 1");
        putEvent(EVT_REFRESH);
        currentCategory = menuVerticalPosition;
      }
    }
  }

  // Models
  if (!error) {
    bool result = storageSeekCategory(&storage, currentCategory);
    coord_t y = MENU_HEADER_HEIGHT+7;
    int count = 0;
    while (result) {
      char line[LEN_MODEL_FILENAME+1];
      result = storageReadNextModel(&storage, line, LEN_MODEL_FILENAME);
      if (!result) {
        break;
      }
      if (count >= menuVerticalOffset*2 && count < (menuVerticalOffset+3)*2) {
        bool selected = (selectMode==MODE_SELECT_MODEL && menuVerticalPosition*2+menuHorizontalPosition==count);
        if (count & 1) {
          drawModel(CATEGORIES_WIDTH+MENUS_MARGIN_LEFT+162, y, line, selected);
          y += 66;
        }
        else {
          drawModel(CATEGORIES_WIDTH+MENUS_MARGIN_LEFT+1, y, line, selected);
        }
        if (selected) {
          memcpy(selectedFilename, line, sizeof(selectedFilename));
        }
      }
      count++;
    }
    if (selectMode == MODE_SELECT_MODEL) {
      if (count == 0) {
        selectMode = MODE_SELECT_CATEGORY;
        menuVerticalPosition = currentCategory;
        putEvent(EVT_REFRESH);
      }
      else if (navigate(event, count, 3, 2)) {
        putEvent(EVT_REFRESH);
      }
    }
    drawVerticalScrollbar(DEFAULT_SCROLLBAR_X, MENU_HEADER_HEIGHT+7, MENU_FOOTER_TOP-MENU_HEADER_HEIGHT-15, menuVerticalOffset, (count+1)/2, 3);
  }

  return true;
}