/**
 * @file prog01/app/src/linkedlist.c
 */

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <user/linkedlist.h>
#include <user/types.h>

#include <stddef.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

UError_t LinkedList_PushBack(LinkedList_t* list, LinkedList_t* add) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == list || NULL == add) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    LinkedList_t* cur = list;
    while (NULL != cur->next) {
      cur = cur->next;
    }
    add->prev = cur;
    add->next = NULL;
    cur->next = add;
  }

  return err;
}

UError_t LinkedList_PushFront(LinkedList_t* list, LinkedList_t* add) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == list || NULL == add) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    LinkedList_t* cur = list->next;
    if (NULL != cur) {
      cur->prev = add;
    }
    list->next = add;
    add->prev = list;
    add->next = cur;
  }

  return err;
}

size_t LinkedList_Count(LinkedList_t* list) {
  UError_t err = uSuccess;
  size_t ret = 0u;

  if (uSuccess == err) {
    if (NULL == list) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    if (NULL != list->next) {
      ++ret;

      LinkedList_t* cur = list->next;
      while (NULL != cur->next) {
        cur = cur->next;
        ++ret;
      }
    }
  }

  return ret;
}

const LinkedList_t* LinkedList_Get(LinkedList_t* list, const size_t pos) {
  UError_t err = uSuccess;
  LinkedList_t* ret = NULL;

  if (uSuccess == err) {
    if (NULL == list) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    ret = list->next;
    for (size_t i = 0; i < pos && NULL != ret; ++i) {
      ret = ret->next;
    }
  }

  return ret;
}

LinkedList_t* LinkedList_PopBack(LinkedList_t* list) {
  UError_t err = uSuccess;
  LinkedList_t* ret = NULL;

  if (uSuccess == err) {
    if (NULL == list) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    if (NULL != list->next) {
      ret = list->next;
      while (NULL != ret->next) {
        ret = ret->next;
      }
      (ret->prev)->next = ret->next;
      ret->prev = NULL;
      ret->next = NULL;
    }
  }

  return ret;
}

LinkedList_t* LinkedList_PopFront(LinkedList_t* list) {
  UError_t err = uSuccess;
  LinkedList_t* ret = NULL;

  if (uSuccess == err) {
    if (NULL == list) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    ret = list->next;

    if (NULL != ret) {
      list->next = ret->next;
      ret->prev = NULL;
      ret->next = NULL;
    }

    if (NULL != list->next) {
      (list->next)->prev = list;
    }
  }

  return ret;
}
