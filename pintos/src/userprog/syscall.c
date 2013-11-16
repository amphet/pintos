#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/filesys.h"

/* This is a skeleton system call handler */

static void syscall_handler (struct intr_frame *);
static int get_user (const uint8_t *uaddr);
static bool put_user (uint8_t *udst, uint8_t byte);

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
	int arg1,arg2,arg3;
	struct thread *t;
	//printf("get_user test : %d\n",	get_user ((uint8_t *)0x10101010));

	switch(syscall){
		case SYS_HALT:// syscall 0
			power_off();			
			break;
		case SYS_EXIT://syscall 1
			f->eax = *((int*)f->esp+1);//int status
			printf("%s: exit(%d)\n",thread_current()->name,f->eax);
			thread_exit();
			break;
		case SYS_CREATE://syscall 4
			arg1 = *((int*)f->esp+1);//char* file
			arg2 = *((int*)f->esp+2);// unsigned initial_size
			filesys_create(arg1,arg2);			
			break;
		case SYS_REMOVE://syscall 5
			arg1 = *((int*)f->esp+1); // char* file 
			filesys_remove(arg1);
			break;
		case SYS_OPEN://syscall 6
			arg1 = *((int*)f->esp+1);//char* file
			struct  file *temp= filesys_open(arg1);
			t = thread_current();
			t->open_file_table[ t->new_fd ] = temp;
			f->eax = t->new_fd;
			t->new_fd++;
			break;
		case SYS_FILESIZE://syscall 7
			arg1 = *((int*)f->esp+1);//int fd
			t = thread_current();
			off_t size;//off_t == int32_t(defined in filesys/off_t.h)
			f->eax = file_length(t->open_file_table[arg1]);
			break;
		case SYS_READ://syscall 8
			arg1 = *((int*)f->esp+1);//int fd
			arg2 = *((int*)f->esp+2);//void* buff
			arg3 = *((int*)f->esp+3);//unsigned size
			if(arg1 == 0) //read from stdin
			{
				* (char*)arg2 = input_getc();
				f->eax = 1;//maybe input_getc() read 1 char from stdin....???
			}
			else if(arg1 == 1) // read from stdin
			{
				//return -1(error case)
				f->eax = -1;			
			}
			else// read from file
			{		
				t = thread_current();
				f->eax = file_read(t->open_file_table[arg1], arg2,arg3);
			}
			break;
		case  SYS_WRITE://syscall 9
			arg1 = *((int*)f->esp+1);//fd
			arg2 = *((int*)f->esp+2);//buff
			arg3 = *((int*)f->esp+3);//size;
			
			
			if(arg1 == 1)//write to stdout
			{
				putbuf ((char*)arg2, arg3);	 
			}
			else if(arg1 == 0)//write to stdin -> error
			{
				f->eax = -1;
			}
			else//write to file
			{
				t = thread_current();
				f->eax = file_write(t->open_file_table[arg1], arg2,arg3);
			}


			
			break;
		case SYS_SEEK://syscall 10
			arg1 = *((int*)f->esp+1);//int fd
			arg2 = *((int*)f->esp+2);//unsigned position
			file_seek (t->open_file_table[arg1], arg2);

			break;
		case SYS_TELL://syscall 11
			break;

		default:
		//	printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n\n");
		//	printf("not implemented(systemcall no.%d) \n",syscall);
		//	printf("call power_off()...\n");
		//	printf("\n\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
			power_off();
			break;
	}
  //hex_dump(0,f->frame_pointer,20,true);




  //thread_exit ();
}


/***********************************************/
//implemented from pintos manual page 26

/* Reads a byte at user virtual address UADDR.
UADDR must be below PHYS_BASE.
Returns the byte value if successful, -1 if a segfault
occurred. */
static int
get_user (const uint8_t *uaddr)
{
	int result;
	asm ("movl $1f, %0; movzbl %1, %0; 1:"
			: "=&a" (result) : "m" (*uaddr));
	return result;
}
/* Writes BYTE to user address UDST.
UDST must be below PHYS_BASE.
Returns true if successful, false if a segfault occurred. */
static bool
put_user (uint8_t *udst, uint8_t byte)
{
	int error_code;
	asm ("movl $1f, %0; movb %b2, %1; 1:"
	: "=&a" (error_code), "=m" (*udst) : "r" (byte));
	return error_code != -1;
}

