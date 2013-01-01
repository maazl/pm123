#define INCL_DOS
#include <os2.h>
#include <process.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <malloc.h>

#include <cpp/smartptr.h>
#include <interlocked.h>
#include <debuglog.h>


// test tracer - used as user data
volatile unsigned inst_counter = 0;
volatile unsigned id_counter = 0;

struct my_data : public Iref_count
{
  const int i;
  const int j;

  my_data(int i) : i(i), j(InterlockedXad(&id_counter, 1))
  { InterlockedInc(&inst_counter);
    DEBUGLOG(("ctor %p %d %d %d\r\n", this, i, j, inst_counter));
  }

  ~my_data()
  { DEBUGLOG(("dtor %p %d %d %d\r\n", this, i, j, inst_counter));
    InterlockedDec(&inst_counter);
  }

};


// And here is the test:

volatile unsigned thread_counter = 0;

static void reader_thread(void* p)
{
  volatile int_ptr<my_data>& s = *(volatile int_ptr<my_data>*)p;
  for (int i = 0; i <= 30000; ++i)
  {
    int_ptr<my_data> h(s);
    // here is our object
    my_data* data = h.get();
    // work with data
    DEBUGLOG(("read %p %d %d\r\n", data, data->i, *(unsigned*)data));
    //Sleep(0);
    // get one more ref but now with normal thread safety
    int_ptr<my_data> h2(h);
    // ...
  }
  InterlockedDec(&thread_counter);
}

static void writer_thread(void* p)
{
  volatile int_ptr<my_data>& s = *(volatile int_ptr<my_data>*)p;
  for (int i = 1; i <= 30000; ++i)
  {
    my_data* data = new my_data(i);
    DEBUGLOG(("repl %p %d %d\r\n", data, i, data->j));
    s = data;
    //Sleep(0);
  }
  InterlockedDec(&thread_counter);
}

static void starter(void (*fn)(void*), volatile void* p)
{ InterlockedInc(&thread_counter);
  RASSERT(_beginthread(fn, NULL, 0, (void*)p) > 0);
}

int main()
{
  {
    volatile int_ptr<my_data> s(new my_data(0));

    starter(reader_thread, &s);
    starter(writer_thread, &s);
    starter(reader_thread, &s);
    starter(writer_thread, &s);
    starter(reader_thread, &s);
    starter(reader_thread, &s);
    starter(writer_thread, &s);
    starter(reader_thread, &s);
    starter(reader_thread, &s);
    starter(reader_thread, &s);

    do DosSleep(100);
      while (thread_counter != 0);
  }

  DEBUGLOG(("dead - %li instances\r\n", inst_counter));
  putchar('\7');
  ASSERT(inst_counter == 0);

  return 0;
}

