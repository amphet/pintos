#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* This is a skeleton system call handler */

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  //printf ("system call!,,, %d\n",*(int*)f->esp);
	int syscall = *(int*)f->esp;

	switch(syscall){
		case  SYS_WRITE:
			//hex_dump(-50,f->eax,100,true);
			//printf("syswrite : \n");
			//printf("fd is %d\n",*((int*)f->esp+1));
			printf("%s",*((int*)f->esp+2));
			//printf("size is %d\n",*((int*)f->esp+3));
			break;
		default:
			printf("not implemented (%d)\n",syscall);
			thread_exit();
			break;
	}
  //hex_dump(0,f->frame_pointer,20,true);




  //thread_exit ();
}
