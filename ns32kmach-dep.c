/* Low level interface for GDB when running under Mach.
   Copyright (C) 1988, 1989 Free Software Foundation, Inc.

This file is part of GDB.

GDB is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GDB is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GDB; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/*
 * HISTORY
 * $Log:	ns32kmach-dep.c,v $
 * Revision 3.2  90/04/11  14:44:48  af
 * 	Moved a bunch of decls in the MI side, where they belong.
 * 
 * 
 */

#include <stdio.h>
#include "defs.h"
#include "param.h"
#include "frame.h"
#include "inferior.h"
#include "symtab.h"
#include "value.h"
#include "wait.h"

#ifdef USG
#include <sys/types.h>
#endif

#include <sys/param.h>
#include <sys/dir.h>
#include <signal.h>
#include <sys/ioctl.h>
/* #include <fcntl.h>  Can we live without this?  */

#ifdef COFF_ENCAPSULATE
#include "a.out.encap.h"
#else
#include <a.out.h>
#endif
#ifndef N_SET_MAGIC
#define N_SET_MAGIC(exec, val) ((exec).a_magic = (val))
#endif

#include <sys/ptrace.h>
#include <errno.h>
/*#include <sys/user.h>		/* After a.out.h  */
#include <sys/file.h>
#include <sys/stat.h>

extern int errno;

#include <mach/message.h>
#include <mach/exception.h>
#include <mach/notify.h>
#include <mach/port.h>
#include <mach/machine/thread_status.h>

#if defined(vax) || defined(i386)
#include <machine/psl.h>
#endif /* vax || i386 */

#include <cthreads.h>
#define	MTHREAD 1
#include <cthread_internals.h>

extern thread_t current_thread;

extern void mach_error();


/* This is one way to do it, not the nicest one */

#include "mach_os.c"


/* Get all registers from the inferior */

extern char registers[];

void
fetch_inferior_registers ()
{
  register int regno;
  register int *regp;
  struct ns32000_thread_state state;
  int stateCnt = NS32000_THREAD_STATE_COUNT;
  kern_return_t ret;  

  if (remote_debugging)
    {
      remote_fetch_registers(registers);
      return;
    }

  if ((ret = thread_get_state(current_thread, NS32000_THREAD_STATE, &state,
			      &stateCnt)) != KERN_SUCCESS) 
    {
      mach_error("thread_get_state", ret);
      error("in fetch_inferior_registers.");
      return;
    }

  bcopy( &state, registers, sizeof(state));

}

/* Store our register values back into the inferior.
   If REGNO is -1, do this for all registers.
   Otherwise, REGNO specifies which register (so we can waste more time).  */

store_inferior_registers (regno)
     register int regno;
{
  register int *regp;
  struct ns32000_thread_state state;
  int stateCnt;
  kern_return_t ret;
  
  if (remote_debugging)
    {
      remote_store_registers(registers);
      return;
    }

  if (regno == -1)
    {
      bcopy(registers, &state, sizeof(state));

      stateCnt = NS32000_THREAD_STATE_COUNT;
      if ((ret = thread_set_state(current_thread, NS32000_THREAD_STATE, &state,
			      	  stateCnt)) != KERN_SUCCESS) 
        {
          mach_error("thread_set_state", ret);
          error("in store_inferior_registers.");
        }
    }
  else
    {
      stateCnt = NS32000_THREAD_STATE_COUNT;
      if ((ret = thread_get_state(current_thread, NS32000_THREAD_STATE, &state,
			      &stateCnt)) != KERN_SUCCESS) 
        {
           mach_error("thread_get_state", ret);
           error("in store_inferior_register.");
        }

      regp = &state.r0;
      if (regno == PS_REGNUM)
      	state.psr = (short)read_register(regno);
      else
        regp[regno] = read_register(regno);

      stateCnt = NS32000_THREAD_STATE_COUNT;
      if ((ret = thread_set_state(current_thread, NS32000_THREAD_STATE, &state,
					stateCnt)) != KERN_SUCCESS) 
        {
          mach_error("thread_set_state", ret);
          error("in store_inferior_register.");
        }
    }
}

/* Return the thread's stack pointer.  Used when mapping
   kernel threads to Mach CThreads */

thread_stack_pointer(th)
	thread_t th;
{
  struct ns32000_thread_state state;
  int stateCnt = NS32000_THREAD_STATE_COUNT;
  if (thread_get_state(th, NS32000_THREAD_STATE, &state,&stateCnt) != KERN_SUCCESS) 
    return 0;
  return state.sp;
}



/* Work with core dump and executable files, for GDB. 
   This code would be in core.c if it weren't machine-dependent. */

#ifndef N_TXTADDR
#define N_TXTADDR(hdr) 0
#endif /* no N_TXTADDR */

#ifndef N_DATADDR
#define N_DATADDR(hdr) hdr.a_text
#endif /* no N_DATADDR */

/* Make COFF and non-COFF names for things a little more compatible
   to reduce conditionals later.  */

#ifdef COFF_FORMAT
#define a_magic magic
#endif

#ifndef COFF_FORMAT
#ifndef AOUTHDR
#define AOUTHDR struct exec
#endif
#endif

extern char *sys_siglist[];


/* Hook for `exec_file_command' command to call.  */

extern void (*exec_file_display_hook) ();
   
/* File names of core file and executable file.  */

extern char *corefile;
extern char *execfile;

/* Descriptors on which core file and executable file are open.
   Note that the execchan is closed when an inferior is created
   and reopened if the inferior dies or is killed.  */

extern int corechan;
extern int execchan;

/* Last modification time of executable file.
   Also used in source.c to compare against mtime of a source file.  */

extern int exec_mtime;

/* Virtual addresses of bounds of the two areas of memory in the core file.  */

extern CORE_ADDR data_start;
extern CORE_ADDR data_end;
extern CORE_ADDR stack_start;
extern CORE_ADDR stack_end;

/* Virtual addresses of bounds of two areas of memory in the exec file.
   Note that the data area in the exec file is used only when there is no core file.  */

extern CORE_ADDR text_start;
extern CORE_ADDR text_end;

extern CORE_ADDR exec_data_start;
extern CORE_ADDR exec_data_end;

/* Address in executable file of start of text area data.  */

extern int text_offset;

/* Address in executable file of start of data area data.  */

extern int exec_data_offset;

/* Address in core file of start of data area data.  */

extern int data_offset;

/* Address in core file of start of stack area data.  */

extern int stack_offset;

#ifdef COFF_FORMAT
/* various coff data structures */

extern FILHDR file_hdr;
extern SCNHDR text_hdr;
extern SCNHDR data_hdr;

#endif /* not COFF_FORMAT */

/* a.out header saved in core file.  */
  
extern AOUTHDR core_aouthdr;

/* a.out header of exec file.  */

extern AOUTHDR exec_aouthdr;

extern void validate_files ();


/* Work with core dump and executable files, for GDB. 
   This code would be in core.c if it weren't machine-dependent. */

/* Recognize COFF format systems because a.out.h defines AOUTHDR.  */
#ifdef AOUTHDR
#define COFF_FORMAT
#endif

#ifndef N_TXTADDR
#define N_TXTADDR(hdr) 0
#endif /* no N_TXTADDR */

#ifndef N_DATADDR
#define N_DATADDR(hdr) hdr.a_text
#endif /* no N_DATADDR */

/* Make COFF and non-COFF names for things a little more compatible
   to reduce conditionals later.  */

#ifdef COFF_FORMAT
#define a_magic magic
#endif

#ifndef COFF_FORMAT
#ifndef AOUTHDR
#define AOUTHDR struct exec
#endif
#endif

extern char *sys_siglist[];


/* Hook for `exec_file_command' command to call.  */

extern void (*exec_file_display_hook) ();
   
/* File names of core file and executable file.  */

extern char *corefile;
extern char *execfile;

/* Descriptors on which core file and executable file are open.
   Note that the execchan is closed when an inferior is created
   and reopened if the inferior dies or is killed.  */

extern int corechan;
extern int execchan;

/* Last modification time of executable file.
   Also used in source.c to compare against mtime of a source file.  */

extern int exec_mtime;

/* Virtual addresses of bounds of the two areas of memory in the core file.  */

extern CORE_ADDR data_start;
extern CORE_ADDR data_end;
extern CORE_ADDR stack_start;
extern CORE_ADDR stack_end;

/* Virtual addresses of bounds of two areas of memory in the exec file.
   Note that the data area in the exec file is used only when there is no core file.  */

extern CORE_ADDR text_start;
extern CORE_ADDR text_end;

extern CORE_ADDR exec_data_start;
extern CORE_ADDR exec_data_end;

/* Address in executable file of start of text area data.  */

extern int text_offset;

/* Address in executable file of start of data area data.  */

extern int exec_data_offset;

/* Address in core file of start of data area data.  */

extern int data_offset;

/* Address in core file of start of stack area data.  */

extern int stack_offset;

#ifdef COFF_FORMAT
/* various coff data structures */

extern FILHDR file_hdr;
extern SCNHDR text_hdr;
extern SCNHDR data_hdr;

#endif /* not COFF_FORMAT */

/* a.out header saved in core file.  */
  
extern AOUTHDR core_aouthdr;

/* a.out header of exec file.  */

extern AOUTHDR exec_aouthdr;

extern void validate_files ();

core_file_command (filename, from_tty)
     char *filename;
     int from_tty;
{
  int val;
  extern char registers[];

  /* Discard all vestiges of any previous core file
     and mark data and stack spaces as empty.  */

  if (corefile)
    free (corefile);
  corefile = 0;

  if (corechan >= 0)
    close (corechan);
  corechan = -1;

  data_start = 0;
  data_end = 0;
  stack_start = STACK_END_ADDR;
  stack_end = STACK_END_ADDR;

  /* Now, if a new core file was specified, open it and digest it.  */

  if (filename)
    {
      filename = tilde_expand (filename);
      make_cleanup (free, filename);
      
      if (have_inferior_p ())
	error ("To look at a core file, you must kill the inferior with \"kill\".");
      corechan = open (filename, O_RDONLY, 0);
      if (corechan < 0)
	perror_with_name (filename);
      /* 4.2-style (and perhaps also sysV-style) core dump file.  */
      {
	struct ptrace_user u;
	int reg_offset;

	val = myread (corechan, &u, sizeof u);
	if (val < 0)
	  perror_with_name (filename);
	data_start = exec_data_start;

	data_end = data_start + u.pt_dsize;
	stack_start = stack_end - u.pt_ssize;
	data_offset = sizeof u;
	stack_offset = data_offset + u.pt_dsize;
	reg_offset = 0;

	bcopy (&u.pt_aouthdr, &core_aouthdr, sizeof (AOUTHDR));
	printf ("Core file is from \"%s\".\n", u.pt_comm);
	if (u.pt_signal > 0)
	  printf ("Program terminated with signal %d, %s.\n",
			u.pt_signal,
			u.pt_signal < NSIG
			? sys_siglist[u.pt_signal]
			: "(undocumented)");

	/* Read the register values out of the core file and store
	   them where `read_register' will find them.  */

	{
	  register int regno;

	  for (regno = 0; regno < NUM_REGS; regno++)
	    {
	      char buf[MAX_REGISTER_RAW_SIZE];

	      val = lseek (corechan, register_addr (regno, reg_offset), 0);
	      if (val < 0)
		perror_with_name (filename);

 	      val = myread (corechan, buf, sizeof buf);
	      if (val < 0)
		perror_with_name (filename);
	      supply_register (regno, buf);
	    }
	}
      }
      if (filename[0] == '/')
	corefile = savestring (filename, strlen (filename));
      else
	{
	  corefile = concat (current_directory, "/", filename);
	}

      set_current_frame ( create_new_frame (read_register (FP_REGNUM),
					    read_pc ()));
      select_frame (get_current_frame (), 0);
      validate_files ();
    }
  else if (from_tty)
    printf ("No core file now.\n");
}

exec_file_command (filename, from_tty)
     char *filename;
     int from_tty;
{
  int val;

  /* Eliminate all traces of old exec file.
     Mark text segment as empty.  */

  if (execfile)
    free (execfile);
  execfile = 0;
  data_start = 0;
  data_end -= exec_data_start;
  text_start = 0;
  text_end = 0;
  exec_data_start = 0;
  exec_data_end = 0;
  if (execchan >= 0)
    close (execchan);
  execchan = -1;

  /* Now open and digest the file the user requested, if any.  */

  if (filename)
    {
      filename = tilde_expand (filename);
      make_cleanup (free, filename);
      
      execchan = openp (getenv ("PATH"), 1, filename, O_RDONLY, 0,
			&execfile);
      if (execchan < 0)
	perror_with_name (filename);

#ifdef COFF_FORMAT
      {
	int aout_hdrsize;
	int num_sections;

	if (read_file_hdr (execchan, &file_hdr) < 0)
	  error ("\"%s\": not in executable format.", execfile);

	aout_hdrsize = file_hdr.f_opthdr;
	num_sections = file_hdr.f_nscns;

	if (read_aout_hdr (execchan, &exec_aouthdr, aout_hdrsize) < 0)
	  error ("\"%s\": can't read optional aouthdr", execfile);

	if (read_section_hdr (execchan, _TEXT, &text_hdr, num_sections,
			      aout_hdrsize) < 0)
	  error ("\"%s\": can't read text section header", execfile);

	if (read_section_hdr (execchan, _DATA, &data_hdr, num_sections,
			      aout_hdrsize) < 0)
	  error ("\"%s\": can't read data section header", execfile);

	text_start = exec_aouthdr.text_start;
	text_end = text_start + exec_aouthdr.tsize;
	text_offset = text_hdr.s_scnptr;
	exec_data_start = exec_aouthdr.data_start;
	exec_data_end = exec_data_start + exec_aouthdr.dsize;
	exec_data_offset = data_hdr.s_scnptr;
	data_start = exec_data_start;
	data_end += exec_data_start;
	exec_mtime = file_hdr.f_timdat;
      }
#else /* not COFF_FORMAT */
      {
	struct stat st_exec;

	val = myread (execchan, &exec_aouthdr, sizeof (AOUTHDR));

	if (val < 0)
	  perror_with_name (filename);

        text_start = N_TXTADDR (exec_aouthdr);
        exec_data_start = N_DATADDR (exec_aouthdr);

	text_offset = N_TXTOFF (exec_aouthdr);
	exec_data_offset = N_TXTOFF (exec_aouthdr) + exec_aouthdr.a_text;

	text_end = text_start + exec_aouthdr.a_text;
        exec_data_end = exec_data_start + exec_aouthdr.a_data;
	data_start = exec_data_start;
	data_end += exec_data_start;

	fstat (execchan, &st_exec);
	exec_mtime = st_exec.st_mtime;
      }
#endif /* not COFF_FORMAT */

      validate_files ();
    }
  else if (from_tty)
    printf ("No exec file now.\n");

  /* Tell display code (if any) about the changed file name.  */
  if (exec_file_display_hook)
    (*exec_file_display_hook) (filename);
}
