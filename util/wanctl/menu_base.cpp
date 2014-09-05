/***************************************************************************
                          menu_base.cpp  -  description
                             -------------------
    begin                : Mon Mar 1 2004
    copyright            : (C) 2004 by David Rokhvarg
    email                : davidr@sangoma.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "menu_base.h"

#define DBG_MENU_BASE 1
#define MAX_NUM_OF_MENU_ITEMS 20

/*
wanpipe_lxdialog --title "the_title" --backtitle "the backtitle" --menu 'menu text.'
                            20 50 2 '' 'tag1' 'item 1' 'tag2' 'item 2' 2> lx_dialog_output
*/

/*
wanpipe_lxdialog --title "the_title" --backtitle "the backtitle" --backmenu 'menu text.'
                            20 50 2 '' 'tag1' 'item 1' 'tag2' 'item 2' 2> lx_dialog_output

*/

menu_base::menu_base()
{
  Debug(DBG_MENU_BASE, ("menu_base::menu_base()\n"));
  //do nothing
}

menu_base::~menu_base()
{
  Debug(DBG_MENU_BASE, ("menu_base::~menu_base()\n"));
  //do nothing
}

int menu_base::set_configuration(
                        IN char menu_type,
                        IN char * lxdialog_path,
                        IN char * title,
                        IN char * backtitle,
                        IN char * explanation_text,
                        IN int hight,
                        IN int width,
                        IN int number_of_items,
                        //number of items may be different
                        ...)
{
  char tmp_buff[MAX_PATH_LENGTH];
	int counter;
	const char *msg;
	va_list ap;

  Debug(DBG_MENU_BASE, ("menu_base::set_configuration() - V1\n"));

  this->menu_type = menu_type;

  snprintf(tmp_buff, MAX_PATH_LENGTH,
    "%s --title \"%s\" --backtitle \"%s\" %s \"%s\" %d %d %d \"\" ",
    lxdialog_path,
    title,
    backtitle,
    (menu_type == MENU_BOX_SELECT ?
      "--menu" :
      "--menuback"),
    explanation_text,
    hight,
    width,
    number_of_items
    );

	va_start(ap, number_of_items);
  counter = 0;

	msg = va_arg(ap, const char *);
  //go through parameter list until end is found
	while(strcmp(msg, END_OF_PARAMS_LIST)){
    Debug(DBG_MENU_BASE, ("msg: %s\n", msg));

    snprintf(&tmp_buff[strlen(tmp_buff)], MAX_PATH_LENGTH, " \"%s\" ", msg);

		msg = va_arg(ap, const char *);
		counter++;

		if(counter > MAX_NUM_OF_MENU_ITEMS) {
			printf("menu_base::set_configuration() : Parameter array is too long !!\n");
			va_end(ap);
			return NO;
		}
	};
	va_end(ap);

  snprintf(&tmp_buff[strlen(tmp_buff)], MAX_PATH_LENGTH, " 2> %s\n", LXDIALOG_OUTPUT_FILE_NAME);

  return set_lxdialog_path((char*)tmp_buff);
}

int menu_base::set_configuration(
                        IN int reserved,
                        IN char menu_type,
                        IN char * lxdialog_path,
                        IN char * title,
                        IN char * backtitle,
                        IN char * explanation_text,
                        IN int hight,
                        IN int width,
                        IN int number_of_items,
                        IN char * tags_and_items_string)
{
  char tmp_buff[MAX_PATH_LENGTH];

  Debug(DBG_MENU_BASE, ("menu_base::set_configuration() - V2\n"));
  Debug(DBG_MENU_BASE, ("number_of_items:%d\n", number_of_items));

  this->menu_type = menu_type;

  snprintf(tmp_buff, MAX_PATH_LENGTH,
    "%s --title \"%s\" --backtitle \"%s\" %s \"%s\" %d %d %d \"\" ",
    lxdialog_path,
    title,
    backtitle,
    (menu_type == MENU_BOX_SELECT ?
      "--menu" :
      "--menuback"),
    explanation_text,
    hight,
    width,
    number_of_items
    );

  Debug(DBG_MENU_BASE, ("tags_and_items_string: %s\n", tags_and_items_string));

  snprintf(&tmp_buff[strlen(tmp_buff)], MAX_PATH_LENGTH, " %s ", tags_and_items_string);

  snprintf(&tmp_buff[strlen(tmp_buff)], MAX_PATH_LENGTH, " 2> %s\n", LXDIALOG_OUTPUT_FILE_NAME);

  return set_lxdialog_path((char*)tmp_buff);
}

int menu_base::show(OUT int * selection_index)
{
  int rc;

  Debug(DBG_MENU_BASE, ("menu_base::show()\n"));

  rc = execute_command_line(selection_index);

  if(rc == NO){
    Debug(DBG_MENU_BASE, ("execute_command_line() returned NO!!\n"));
    return rc;
  }

  switch(*selection_index)
  {
  case MENU_BOX_BUTTON_SELECT:
    Debug(DBG_MENU_BASE, ("MENU_BOX_BUTTON_SELECT\n"));
    rc = handle_selection();
    break;

  case MENU_BOX_BUTTON_EXIT:
    //do nothing. only return 'YES' and indicate that 'Exit' was selected.
    switch(menu_type)
    {
    case MENU_BOX_SELECT:
      Debug(DBG_MENU_BASE, ("MENU_BOX_BUTTON_EXIT\n"));
      break;

    case MENU_BOX_BACK:
      Debug(DBG_MENU_BASE, ("MENU_BOX_BUTTON_BACK\n"));
      break;
    }

    break;

  case MENU_BOX_BUTTON_HELP:
    //do nothing. only return 'YES' and indicate that 'Help' was selected.
    Debug(DBG_MENU_BASE, ("MENU_BOX_BUTTON_HELP\n"));

    rc = handle_selection();
    break;

  default:
    printf("Invalid selection for 'Menu Box'!!\n");
    rc = NO;
  }

  return rc;
}

int menu_base::handle_selection()
{
  int rc = YES;

  if(open_lxdialog_output_file(LXDIALOG_OUTPUT_FILE_NAME) == NO){
    rc = NO;
    goto done;
  }

  if(read_lxdialog_output_file() == NO){
    rc = NO;
    goto done;
  }
done:
  return rc;
}
