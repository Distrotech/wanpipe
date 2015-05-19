/***************************************************************************
                          objects_list.h  -  description
                             -------------------
    begin                : Fri Mar 5 2004
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

#ifndef OBJECTS_LIST_H
#define OBJECTS_LIST_H

#define DBG_OBJ_LIST  0

enum LIST_ACTION_RETURN_CODE{

  LIST_ACTION_OK=1,
  LIST_INSERTION_FAILED_ELEMENT_EXIST,
  LIST_INSERTION_FAILED_GENERAL_ERROR
};

#define DECODE_LIST_ACTION_RC(rc)		\
(						\
(rc == LIST_ACTION_OK) ? "LIST_ACTION_OK" :	\
(rc == LIST_INSERTION_FAILED_ELEMENT_EXIST) ? "LIST_INSERTION_FAILED_ELEMENT_EXIST" :	\
(rc == LIST_INSERTION_FAILED_GENERAL_ERROR) ? "LIST_INSERTION_FAILED_GENERAL_ERROR" : "Unknown")

#include "list_element.h"
#include "list_element_chan_def.h"

#if defined(ZAPTEL_PARSER)
#include "list_element_sangoma_card.h"
#endif

/**
  *@author David Rokhvarg
  */

class objects_list {

  unsigned int size;
  list_element* head;
  list_element* tail;

public:
  void remove_all_elements()
  {
    Debug(DBG_OBJ_LIST, ("remove_all_elements()\n"));
    list_element* tmp;
    list_element_chan_def* tmp_chan_def;
	
    while((tmp = remove_from_tail()) != NULL){

      switch(tmp->get_element_type())
      {
      case CHAN_DEF_LIST_ELEMENT:
	tmp_chan_def = (list_element_chan_def*)tmp;
	
	//if there is something above this level, delete it too
	if(tmp_chan_def->next_objects_list != NULL){
	  delete (objects_list*)tmp_chan_def->next_objects_list;
	  tmp_chan_def->next_objects_list = NULL;
	}
        delete tmp_chan_def;
        break;

#if defined(ZAPTEL_PARSER)
      case SANGOMA_CARD_LIST_ELEMENT:
        delete (list_element_sangoma_card*)tmp;
	break;
#endif

      default:
        Debug(1, ("remove_all_elements() - deleting 'list_element' of unknown type: %d!!\n",
          tmp->get_element_type()));
        delete tmp;
        break;
      }
    }
    Debug(DBG_OBJ_LIST, ("remove_all_elements()- End\n"));

    size = 0;
    tail = NULL;
    head = NULL;
  }

//public:

  objects_list()
  {
    Debug(DBG_OBJ_LIST, ("objects_list::objects_list()\n"));

    size = 0;
    tail = NULL;
    head = NULL;
  }

  ~objects_list()
  {
    Debug(DBG_OBJ_LIST, ("objects_list::~objects_list()\n"));
    remove_all_elements();
  }

  //insert using insertion sort, so largest element will be always at the head,
  //smallest at the tail.
  int insert(list_element* new_element)
  {
    int rc;

    Debug(DBG_OBJ_LIST, ("objects_list::insert()\n"));

    rc = real_insert(new_element);

#if 0
    //for debugging display content after each insertion.
    display_contents_of_list();
#endif

    return rc;
  }

  //this function prints out contents of only one (1) level
  //for this 'objects_list' object.
  void display_contents_of_list()
  {
    printf("------display_contents_of_list()-----------\n");
    list_element_chan_def* tmp = (list_element_chan_def*)get_first();
    while(tmp != NULL){
      printf("comparator: %s\n", tmp->get_comparator());
      tmp = (list_element_chan_def*)get_next_element(tmp);
    }
    printf("-------------------------------------------\n");
  }

  //this function prints out contents of *all* levels
  //of any 'objects_list' object .
  static void print_obj_list_contents(objects_list* the_list)
  {
    chan_def_t* chandef;
      
    if(the_list != NULL){
  
      list_element_chan_def* list_el_chan_def = (list_element_chan_def*)the_list->get_first();

      //each link may have pointers to other "conf_file_reader" objects. print them all.
      while(list_el_chan_def != NULL){

	chandef = &list_el_chan_def->data;

	printf("device name->: %s\n",chandef->name );
	
	if(list_el_chan_def->next_objects_list != NULL){
	  printf("-------- next level -------\n" );
	  //recursive call
	  print_obj_list_contents((objects_list*)list_el_chan_def->next_objects_list);
	  printf("-------- end of next level -------\n");
	}

	list_el_chan_def = (list_element_chan_def*)the_list->get_next_element(list_el_chan_def);
      }//while()
    }//if()
  }
  
  int real_insert(list_element* new_element)
  {
    Debug(DBG_OBJ_LIST, ("objects_list::real_insert()\n"));

    new_element->reset();

    //special case - list is empty
    if(tail == NULL){
      Debug(DBG_OBJ_LIST, ("special case - list is empty\n"));
      tail = head = new_element;

      size++;

      Debug(DBG_OBJ_LIST, ("size:%d\n", size));
      return LIST_ACTION_OK;
    }

    list_element* tmp_element = tail;

    while(tmp_element != NULL){

      if(tmp_element->compare(new_element) < 0){

        list_element* next_element = get_next_element(tmp_element);

        //check the next element greater than the new one.
        if( next_element != NULL){

          if(next_element->compare(new_element) > 0){
            //this works because the list is *always* kept in order
            insert_after_element(tmp_element, new_element);
            return LIST_ACTION_OK;
          }else{
            //go to next
          }

        }else{
          //it is the last element insert after it.
          Debug(DBG_OBJ_LIST, ("inserting after last element\n"));
          head = new_element;
          insert_after_element(tmp_element, new_element);
          return LIST_ACTION_OK;
        }

      }else if(tmp_element->compare(new_element) == 0){
        Debug(DBG_OBJ_LIST, ("attempting to insert existing element %s!!\n",
          tmp_element->get_comparator()));
        return LIST_INSERTION_FAILED_ELEMENT_EXIST;

      }else if(tmp_element->compare(new_element) > 0){

        insert_before_element(tmp_element, new_element);
        return LIST_ACTION_OK;
      }

      tmp_element = get_next_element(tmp_element);
    }//while()

    Debug(DBG_OBJ_LIST, ("insert() failer!!\n"));
    return LIST_INSERTION_FAILED_GENERAL_ERROR;
  }

  list_element* get_next_element(list_element* element)
  {
    Debug(DBG_OBJ_LIST, ("objects_list::get_next_element()\n"));

    return (list_element*)element->next;
  }


  list_element* get_previous_element(list_element* element)
  {
    Debug(DBG_OBJ_LIST, ("objects_list::get_previous_element()\n"));

    return (list_element*)element->previous;
  }

  void insert_after_element(list_element* element, list_element* new_element)
  {
    Debug(DBG_OBJ_LIST, ("objects_list::insert_after_element()\n"));
    Debug(DBG_OBJ_LIST, ("element: %s, new_element: %s\n",
      element->get_comparator(), new_element->get_comparator() ));

    new_element->reset();

    new_element->next = element->next;

    if(new_element->next != NULL){
      ((list_element*)new_element->next)->previous = new_element;
    }

    element->next = new_element;

    new_element->previous = element;

    size++;

    Debug(DBG_OBJ_LIST, ("insert_after_element(): size:%d\n", size));
  }

  void insert_before_element(list_element* element, list_element* new_element)
  {
    Debug(DBG_OBJ_LIST, ("objects_list::insert_before_element()\n"));
    Debug(DBG_OBJ_LIST, ("element: %s, new_element: %s\n",
      element->get_comparator(), new_element->get_comparator() ));

    new_element->reset();

    new_element->next = element;

    new_element->previous = element->previous;

    if(new_element->previous != NULL){
      ((list_element*)new_element->previous)->next = new_element;
    }
    element->previous = new_element;

    //special case inserting before the first element
    if(tail == element){
      tail = new_element;
    }

    size++;
    Debug(DBG_OBJ_LIST, ("insert_before_element(): size:%d\n", size));
  }

  list_element* get_first()
  {
    Debug(DBG_OBJ_LIST, ("objects_list::get_first()\n"));

    return tail;
  }

  list_element* get_last()
  {
    Debug(DBG_OBJ_LIST, ("objects_list::get_last()\n"));

    return head;
  }

  unsigned int get_size()
  {
    return size;
  }

  //returns pointer to the smallest element,
  //removes it from the list.
  list_element* remove_from_tail()
  {
    if(tail == NULL){
      Debug(DBG_OBJ_LIST, ("remove_from_tail(): list is empty.\n"));
      return NULL;
    }

    list_element* tmp = tail;
    list_element* next = (list_element*)tmp->next;

    tail = next;
    if(tail != NULL){
      tail->previous = NULL;
    }else{
      Debug(DBG_OBJ_LIST, ("remove_from_tail(): removed last element.\n"));
      head = NULL;
    }

    size--;
    Debug(DBG_OBJ_LIST, ("remove_from_tail(): size: %d\n", size));
    return tmp;
  }

  //returns pointer to the largest element,
  //removes it from the list.
  list_element* remove_from_head()
  {
    if(head == NULL){
      Debug(DBG_OBJ_LIST, ("remove_from_head(): list is empty.\n"));
      return NULL;
    }

    list_element* tmp = head;
    list_element* previous = (list_element*)tmp->previous;

    head = previous;
    if(head != NULL){
      head->next = NULL;
    }else{
      Debug(DBG_OBJ_LIST, ("remove_from_head(): removed last element.\n"));
      tail = NULL;
    }

    size--;
    Debug(DBG_OBJ_LIST, ("remove_from_head(): size: %d\n", size));
    return tmp;
  }

  //returns pointer to the element containing 'comparator',
  //removes the element from the list.
  list_element* remove_element(char* comparator)
  {
    Debug(DBG_OBJ_LIST, ("remove_element(): searching for comparator: %s\n", comparator));

    list_element* tmp = find_element(comparator);
    if(tmp == NULL){
      //no such element
      return NULL;
    }

    list_element* previous = (list_element*)tmp->previous;
    list_element* next = (list_element*)tmp->next;

    if(previous != NULL){
      previous->next = next;
    }else{
      //element at the tail is being deleted
      tail = next;
      if(next != NULL){
        next->previous = NULL;
      }
    }

    if(next != NULL){
      next->previous = previous;
    }else{
      //element at the head is being deleted
      head = previous;
      if(previous != NULL){
        previous->next = NULL;
      }
    }

    size--;

    if(size == 0){
      head = NULL;
      tail = NULL;
    }
    return tmp;
  }

  //returns pointer to the element containing 'comparator',
  //if found
  list_element* find_element(char* comparator)
  {
    Debug(DBG_OBJ_LIST, ("find_element(char* comparator): searching for comparator: %s\n",
      comparator));

    list_element* tmp = get_first();

    while(tmp != NULL){
      if(tmp->compare(comparator) == 0){
        Debug(DBG_OBJ_LIST, ("find_element(): found searched element in the list %s\n", comparator));
        return tmp;
      }
      tmp = get_next_element(tmp);
    }

    Debug(DBG_OBJ_LIST, ("find_element(): DID NOT found in the list %s!!\n", comparator));
    return NULL;
  }
/*
  list_element* find_element(int comparator)
  {
    Debug(DBG_OBJ_LIST, ("find_element(int comparator): searching for comparator: %d\n",
      comparator));

    list_element* tmp = get_first();

    while(tmp != NULL){
      if(tmp->compare(comparator) == 0){
        Debug(DBG_OBJ_LIST, ("find_element(): found searched element in the list %s\n", comparator));
        return tmp;
      }
      tmp = get_next_element(tmp);
    }

    Debug(DBG_OBJ_LIST, ("find_element(): DID NOT found in the list %d!!\n", comparator));
    return NULL;
  }
*/
};

#endif
