/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 *    3. The name of the author may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>

typedef struct LIST_NODE_STRUCT
{
   void *data;
   struct LIST_NODE_STRUCT *next;

} LIST_NODE;

LIST_NODE *list_create(void)
{
   LIST_NODE *head = malloc(sizeof(LIST_NODE));
   if(head == NULL)
      return NULL;

   head->data = NULL;
   head->next = NULL;

   return head;
}

int list_destroy(LIST_NODE *head)
{
   LIST_NODE *node = head;
   if(node == NULL)
      return -1;
   while(node->next != NULL)
   {
      node = node->next;
      free(node);
   }
   return 0;
}

LIST_NODE *list_add(LIST_NODE *head, void *data)
{
   LIST_NODE *node = head;
   if(node == NULL)
      return NULL;
   while(node->next != NULL)
   {
      node = node->next;
   }

   node->next = malloc(sizeof(LIST_NODE));
   if(node->next == NULL)
      return NULL;

   node->next->next = NULL;
   node->next->data = data;

   return node->next;
}

LIST_NODE *list_search(LIST_NODE *head, void *data)
{
   LIST_NODE *node;
   if(head == NULL)
      return NULL;
   node = head->next;
   while(node != NULL && node->data != data)
   {
      node = node->next;
   }

   if(node != NULL && node->data == data)
      return node;
   else
      return NULL;
}

int list_remove(LIST_NODE *head, void *data)
{
   LIST_NODE *prev_node = NULL;
   LIST_NODE *node = NULL;
   if(head == NULL)
      return -1; // error
   prev_node = head;
   while(prev_node->next != NULL && prev_node->next->data != data)
   {
      prev_node = prev_node->next;
   }

   if(prev_node->next == NULL)
      return 0; // not found

   node = prev_node->next;

   prev_node->next = node->next;
   free(node);

   return 1;
}

int list_isempty(LIST_NODE *head)
{
   if(head == NULL)
      return -1; // error
   else if(head->next == NULL)
      return 1; // is empty
   else
      return 0; // is not empty
}

LIST_NODE *list_getnext(LIST_NODE *node)
{
   return node->next;
}

void *list_getdata(LIST_NODE *node)
{
   return node->data;
}
