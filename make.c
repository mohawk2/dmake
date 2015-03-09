/*
--
-- SYNOPSIS
--      Perform the update of all outdated targets.
-- 
-- DESCRIPTION
--	This is where we traverse the make graph looking for targets that
--	are out of date, and we try to infer how to make them if we can.
--	The usual make macros are understood, as well as some new ones:
--
--		$$	- expands to $
--		$@      - full target name
--		$*      - target name with no suffix, same as $(@:db)
--			  or, the value of % in % meta rule recipes
--		$?      - list of out of date prerequisites
--		$<      - all prerequisites associated with rules line
--		$&	- all prerequisites associated with target
--		$>      - library name for target (if any)
--		$^	- out of date prerequisites taken from value of $<
-- 
-- AUTHOR
--      Dennis Vadura, dvadura@dmake.wticorp.com
--
-- WWW
--      http://dmake.wticorp.com/
--
-- COPYRIGHT
--      Copyright (c) 1996,1997 by WTI Corp.  All rights reserved.
-- 
--      This program is NOT free software; you can redistribute it and/or
--      modify it under the terms of the Software License Agreement Provided
--      in the file <distribution-root>/readme/license.txt.
--
-- LOG
--      Use cvs log to obtain detailed change logs.
*/

#include "extern.h"
#include "sysintf.h"

typedef struct cell {
   char            *datum;
   struct  cell    *next;
   size_t           len;
} LISTCELL, *LISTCELLPTR;

typedef struct {
   LISTCELLPTR  first;
   LISTCELLPTR  last;
   size_t       len;
} LISTSTRING, *LISTSTRINGPTR;


static	void	_drop_mac ANSI((HASHPTR));
static	void	_set_recipe ANSI((char*, int));
static	void	_set_tmd ANSI(());
static	void	_append_file ANSI((STRINGPTR, FILE*, char*, int));
static  LINKPTR _dup_prq ANSI((LINKPTR));
static  LINKPTR _expand_dynamic_prq ANSI(( LINKPTR, LINKPTR, char * ));
static  char*   _prefix ANSI((char *, char *));
static  char*   _pool_lookup ANSI((char *));
static  int	_explode_graph ANSI((CELLPTR, LINKPTR, CELLPTR));


#define RP_GPPROLOG       0
#define RP_RECIPE         1
#define RP_GPEPILOG       2
#define NUM_RECIPES       3

static STRINGPTR _recipes[NUM_RECIPES];
static LISTCELLPTR _freelist=NULL;

static LISTCELLPTR
get_cell()
{
   LISTCELLPTR cell;

   if (!_freelist) {
      if ((cell=MALLOC(1,LISTCELL)) == NULL)
         No_ram();
   }
   else {
      cell = _freelist;
      _freelist = cell->next;
   }

   return(cell);
}


static void
free_cell(LISTCELLPTR cell)
{
   cell->next = _freelist;
   _freelist = cell;
}


static void
free_list(LISTCELLPTR c)
{
   if(c) {
      free_list(c->next);
      free_cell(c);
   }
}
   
   
static void
list_init(LISTSTRINGPTR s)
{
   s->first = NULL;
   s->last = NULL;
   s->len = 0;
}


static void
list_add(LISTSTRINGPTR s, char *str)
{
   LISTCELLPTR p;
   int l;

   if ((l = strlen(str)) == 0)
      return;

   p = get_cell();
   p->datum = str;
   p->next  = NULL;
   p->len   = l;

   if(s->first == NULL)
      s->first = p;
   else
      s->last->next = p;

   s->last = p;
   s->len += l+1;
}


static char *
gen_path_list_string(LISTSTRINGPTR s)/*
=======================================
   Take a list of filepaths and create a string from it separating
   the filenames by a space.
   This function honors the cygwin specific .WINPATH attribute. */
{
   LISTCELLPTR next, cell;
   int         len;
   int         slen, slen_rest;
   char        *result;
   char        *p, *tpath;

   if( (slen_rest = slen = s->len) == 0)
      return(NIL(char));

   /* reserve enough space to hold the concated original filenames. */
   if((p = result = MALLOC(slen, char)) == NULL) No_ram();

   for (cell=s->first; cell; cell=next) {
#if !defined(__CYGWIN__)
      tpath = cell->datum;
      len=cell->len;
#else
      /* For cygwin with .WINPATH set the lenght of the converted
       * filepaths might be longer. Extra checking is needed ... */
      tpath = DO_WINPATH(cell->datum);
      if( tpath == cell->datum ) {
	 len=cell->len;
      }
      else {
	 /* ... but only if DO_WINPATH() did something. */
	 len = strlen(tpath);
      }
      if( len >= slen_rest ) {
	 /* We need more memory. As DOS paths are usually shorter than the
	  * original cygwin POSIX paths (exception mounted paths) this should
	  * rarely happen. */
	 int p_offset = p - result;
	 /* Get more than needed. */
	 slen = slen + len - slen_rest + 128;
	 if((result = realloc( result, slen ) ) == NULL)
	    No_ram();
	 p = result + p_offset;
      }
#endif

      memcpy((void *)p, (void *)tpath, len);
      p += len;
      *p++ = ' ';

#if defined(__CYGWIN__)
      /* slen_rest is only used in the cygwin / .WINPATH case. */
      slen_rest = slen - (p - result);
#endif

      next = cell->next;
      free_cell(cell);
   }

   *--p = '\0';
   list_init(s);
   
   return(result);
}


PUBLIC int
Make_targets()/*
================
   Actually go and make the targets on the target list */
{
   LINKPTR lp;
   int     done = 0;

   DB_ENTER( "Make_targets" );

   Read_state();
   _set_recipe( ".GROUPPROLOG", RP_GPPROLOG );
   _set_recipe( ".GROUPEPILOG", RP_GPEPILOG );

   /* Prevent recipe inference for .ROOT */
   if ( Root->ce_recipe == NIL(STRING) ) {
      TALLOC( Root->ce_recipe, 1, STRING );
      Root->ce_recipe->st_string = "";
   }

   /* Prevent recipe inference for .TARGETS */
   if ( Targets->ce_recipe == NIL(STRING) ) {
      TALLOC( Targets->ce_recipe, 1, STRING );
      Targets->ce_recipe->st_string = "";
   }

   /* Make sure that user defined targets are marked as root targets */
   for( lp = Targets->ce_prq; lp != NIL(LINK); lp = lp->cl_next )
      lp->cl_prq->ce_attr |= A_ROOT;

   while( !done ) {
      int rval;

      if( (rval = Make(Root, NIL(CELL))) == -1 )
	 DB_RETURN(1);
      else
	 done = Root->ce_flag & F_MADE;

      if( !rval && !done ) Wait_for_child( FALSE, -1 );
   }

   for( lp = Targets->ce_prq; lp != NIL(LINK); lp = lp->cl_next ) {
      CELLPTR tgt = lp->cl_prq;
      if( !(tgt->ce_attr & A_UPDATED) 
          && (Verbose & V_MAKE) )
          printf( "`%s' is up to date\n", tgt->CE_NAME );
   }

   DB_RETURN( 0 );
}



PUBLIC int
Make( cp, setdirroot )/*
========================
   Make target cp. Make() is also called on prerequisites that have no rule
   associated (F_TARGET is not set) to verify that they exist. */
CELLPTR cp;
CELLPTR setdirroot;
{
   register LINKPTR dp, prev,next;
   register CELLPTR tcp;
   CELLPTR          nsetdirroot;
   char		    *name, *lib;
   HASHPTR	    m_at, m_q, m_b, m_g, m_l, m_bb, m_up;
   LISTSTRING       all_list, imm_list, outall_list, inf_list;
   char             *all    = NIL(char);
   char             *inf    = NIL(char);
   char             *outall = NIL(char);
   char             *imm    = NIL(char);
   int              rval    = 0; /* 0==ready, 1==target still running, -1==error */
   int		    push    = 0;
   int 		    made    = F_MADE;
   int		    ignore;
   time_t           otime   = (time_t) 1L; /* Hold time of newest prerequisite. */
   int		    mark_made = FALSE;

#if defined(__CYGWIN__)
   /* static variable to hold .WINPATH status of previously made target.
    * 0, 1 are .WINPATH states, -1 indicates the first target. */
   static int prev_winpath_attr = -1;
#endif

   DB_ENTER( "Make" );
   DB_PRINT( "mem", ("%s:-> mem %ld", cp->CE_NAME, (long) coreleft()) );

   /* Initialize the various temporary storage */
   m_q = m_b = m_g = m_l = m_bb = m_up = NIL(HASH);
   list_init(&all_list);
   list_init(&imm_list);
   list_init(&outall_list);
   list_init(&inf_list);
   
   if (cp->ce_set && cp->ce_set != cp) {
      if( Verbose & V_MAKE )
	 printf( "%s:  Building .UPDATEALL representative [%s]\n", Pname,
	 	 cp->ce_set->CE_NAME );
      cp = cp->ce_set;
   }

   /* If we are supposed to change directories for this target then do so.
    * If we do change dir, then modify the setdirroot variable to reflect
    * that fact for all of the prerequisites that we will be making. */

   nsetdirroot = setdirroot;
   ignore = (((cp->ce_attr|Glob_attr)&A_IGNORE) != 0);

   /* Set the UseWinpath variable to reflect the (global/local) .WINPATH
    * attribute. The variable is used by DO_WINPATH() and in some other
    * places. */
#if defined(__CYGWIN__)
   UseWinpath = (((cp->ce_attr|Glob_attr)&A_WINPATH) != 0);
#endif

   /* m_at needs to be defined before going to a "stop_making_it" where
    * a _drop_mac( m_at ) would try to free it. */
   /* FIXME: m_at can most probably not be changed before the next
    * Def_macro("@", ...) command. Check if both this and the next
    * call are needed. */
   m_at = Def_macro("@", DO_WINPATH(cp->ce_fname), M_MULTI);

   if( cp->ce_attr & A_SETDIR ) {
      /* Change directory only if the previous .SETDIR is a different
       * directory from the current one.  ie. all cells with the same .SETDIR
       * attribute are assumed to come from the same directory. */

      if( (setdirroot == NIL(CELL) || setdirroot->ce_dir != cp->ce_dir) &&
          (push = Push_dir(cp->ce_dir,cp->CE_NAME,ignore)) != 0 )
	 setdirroot = cp;
   }

   DB_PRINT( "mem", ("%s:-A mem %ld", cp->CE_NAME, (long) coreleft()) );

   /* FIXME: F_MULTI targets don't have cp->ce_recipe set but the recipes
    * are known nevertheless. It is not necessary to infer them.
    * If (cp->ce_flag & F_MULTI) is true the recipes of the corresponding
    * subtargets can be used. */
   if( cp->ce_recipe == NIL(STRING) ) {
      char *dir = cp->ce_dir;
      
      if( Verbose & V_MAKE )
	 printf( "%s:  Infering prerequisite(s) and recipe for [%s]\n", Pname,
	 	 cp->CE_NAME );

      Infer_recipe( cp, setdirroot );

      /* See if the directory has changed, if it has then make sure we
       * push it. */
      if( dir != cp->ce_dir ) {
	 if( push ) Pop_dir(FALSE);
         push = Push_dir( cp->ce_dir, cp->CE_NAME, ignore );
	 setdirroot = cp;
      }
   }

   for(dp=CeMeToo(cp); dp; dp=dp->cl_next) {
      tcp = dp->cl_prq;
      if( push ) {
	 /* If we changed the directory because of .SETDIR write Pwd into
	  * tcp->ce_dir so that it holds an absolute path. */
	 if( !(tcp->ce_attr & A_POOL) && tcp->ce_dir ) FREE( tcp->ce_dir );
	 tcp->ce_dir   = _pool_lookup(Pwd);
	 tcp->ce_attr |= A_SETDIR|A_POOL;
      }
      tcp->ce_setdir = nsetdirroot;
   }

   DB_PRINT( "mem", ("%s:-A mem %ld", cp->CE_NAME, (long) coreleft()) );
   /* If we have not yet statted the target then do so. */
   if( !(cp->ce_flag & F_STAT) && !(cp->ce_attr&A_PHONY) ) {
      if (cp->ce_parent && (cp->ce_parent->ce_flag & F_MULTI)) {
	 /* Inherit the stat info from the F_MULTI parent. */
	 cp->ce_time  = cp->ce_parent->ce_time;
	 cp->ce_flag |= F_STAT;
	 /* Propagate the A_PRECIOUS attribute from the parent. */
	 cp->ce_attr |= cp->ce_parent->ce_attr & A_PRECIOUS;
      }
      else {
	 for(dp=CeMeToo(cp); dp; dp=dp->cl_next) {
	    tcp = dp->cl_prq;
	    /* Check if target already exists. */
	    Stat_target( tcp, 1, FALSE );

	    if( tcp->ce_time != (time_t)0L ) {
	       /* File exists so don't remove it later. */
	       tcp->ce_attr |= A_PRECIOUS;
	    }
	       
	    if( Verbose & V_MAKE )
	       printf("%s:  Time stamp of [%s] is %ld\n",Pname,tcp->CE_NAME,
		      tcp->ce_time);
	 }
      }
   }

   DB_PRINT( "make", ("(%s, %ld, 0x%08x, 0x%04x)", cp->CE_NAME,
			cp->ce_time, cp->ce_attr, cp->ce_flag) );

   /* Handle targets without rule and without existing file. */
   if( !(cp->ce_flag & F_TARGET) && (cp->ce_time == (time_t) 0L) ) {
      if( Makemkf ) {
	 rval = -1;
	 goto stop_making_it;
      }
      else if( cp->ce_prq != NIL(LINK)
            || (BTOBOOL(Augmake) && (cp->ce_flag&F_EXPLICIT)))
	 /* Assume an empty recipe for a target that we have run inference on
	  * but do not have a set of rules for but for which we have inferred
	  * a list of prerequisites. */
	 cp->ce_flag |= F_RULES;
      else
	 Fatal( "`%s' not found, and can't be made", cp->CE_NAME );
   }

   DB_PRINT( "mem", ("%s:-A mem %ld", cp->CE_NAME, (long) coreleft()) );

   /* set value of $* if we have not infered a recipe, in this case $* is
    * the same as $(@:db), this allows us to be compatible with BSD make */
   if( cp->ce_per == NIL(char) ) cp->ce_per = "$(@:db)";

   /* Search the prerequisite list for dynamic prerequisites and if we find
    * them copy the list of prerequisites for potential later re-use. */
   if ( cp->ce_prqorg == NIL(LINK) ) {
      for( dp = cp->ce_prq; dp != NIL(LINK); dp = dp->cl_next )
	 if ( strchr(dp->cl_prq->CE_NAME, '$') != NULL )
	    break;

      if (dp != NIL(LINK)) {
	 cp->ce_prqorg = _dup_prq(cp->ce_prq);
      }
   }

   /* Define $@ macro. The only reason for defining it here (that I see ATM)
    * is that $@ is already defined in conditional macros. */
   /* FIXME: check if both this and the previous Def_macro("@", ...) call
    * are needed. */
   m_at = Def_macro("@", DO_WINPATH(cp->ce_fname), M_MULTI);

   /* Define conditional macros if any, note this is done BEFORE we process
    * prerequisites for the current target.  Thus the making of a prerequisite
    * is done using the current value of the conditional macro. */
   for(dp=CeMeToo(cp); dp; dp=dp->cl_next) {
      tcp=dp->cl_prq;
      if (tcp->ce_cond != NIL(STRING)) {
	 STRINGPTR sp;

	 tcp->ce_pushed = NIL(HASH);
	 for(sp=tcp->ce_cond; sp; sp=sp->st_next) {
	    if(Parse_macro(sp->st_string,M_MULTI|M_PUSH)) {
	       HASHPTR hp;

	       hp = GET_MACRO(LastMacName);
	       hp->ht_link = tcp->ce_pushed;
	       tcp->ce_pushed = hp;
	    }
	    else {
	      Error("Invalid conditional macro expression [%s]",sp->st_string);
	    }
	 }
      }
   }

   /* First round, will be repeated a second time below. */
   for( prev=NULL,dp=cp->ce_prq; dp != NIL(LINK); prev=dp, dp=next ) {
      int seq;

      /* This loop executes Make() to build prerequisites if needed. 
       * The only macro that needs to be reset after a Make() was executed
       * is $@ as it might be used when expanding potential dynamic
       * prerequisites. As UseWinpath is a global variable we also
       * need to restore it. */
      if (m_at->ht_value == NIL(char)) {
	 /* This check effectively tests if Make() was run before because
	  * Make() frees all dynamic macro values at the end. */
#if defined(__CYGWIN__)
	 UseWinpath = (((cp->ce_attr|Glob_attr)&A_WINPATH) != 0);
#endif
	 m_at = Def_macro("@", DO_WINPATH(cp->ce_fname), M_MULTI);
      }

      /* Make the prerequisite, note that if the current target has the
       * .LIBRARY attribute set we pass on to the prerequisite the .LIBRARYM
       * attribute and pass on the name of the current target as the library
       * name, and we take it away when we are done.  */
      next = dp->cl_next;

      tcp = dp->cl_prq;
      if( Verbose & V_MAKE )
	 printf("Checking prerequisite [%s]\n", tcp->CE_NAME);

      seq = (((cp->ce_attr | Glob_attr) & A_SEQ) != 0);

      /* This checks if this prerequisite is still in the making, if yes
       * come back later. */
      if( tcp->ce_flag & F_VISITED ) {
	 /* Check if this currently or fully made target has the same
	  * .SETDIR setting. If yes, continue if it was made or come
	  * back later otherwise. */
	 if( _explode_graph(tcp, dp, setdirroot) == 0 ) {
	    /* didn't blow it up so see if we need to wait for it. */
	    if( tcp->ce_flag & F_MADE ) {
	       /* Target was made. */
	       continue;
	    }
	    else
	       /* Target is still in the making ... */
	       goto stop_making_it;
	 }
	 else
	    /* Use the new prerequisite with the new .SETDIR value. */
	    tcp = dp->cl_prq;
      }

      /* If the previous target (prereq) is not yet ready return if
       * seq is TRUE. */
      if( seq && !made ) goto stop_making_it;

      /* Expand dynamic prerequisites. The F_MARK flag is guarging against
       * possible double expandion of dynamic prerequisites containing more
       * than one prerequisite. */
      /* A new A_DYNAMIC attribute could save a lot of strchr( ,'$') calls. */
      if ( tcp && !(tcp->ce_flag & F_MARK) && strchr(tcp->CE_NAME, '$') ) {
	 /* Replace this dynamic prerequisite with the real prerequisite,
	  * and add the additional prerequisites if there are more than one.*/

	 name = Expand( tcp->CE_NAME );
	 if( strcmp(name,cp->CE_NAME) == 0 )
	    Fatal("Detected circular dynamic dependency; generated '%s'",name);

	 /* Call helper for dynamic prerequisite expansion to replace the
	  * prerequisite with the expanded version and add the new
	  * prerequisites, if the macro expanded to more than one, after
	  * the current list element. */
	 dp = _expand_dynamic_prq( cp->ce_prq, dp, name );
	 FREE( name );

	 /* _expand_dynamic_prq() probably changed dp->cl_prq. */
	 tcp = dp->cl_prq;
	 if ( tcp ) {
	    next = dp->cl_next;
	 }
      }

      /* Dynamic expansion results in a NULL cell only when the new
       * prerequisite is already in the prerequisite list or empty. In this
       * case delete the cell and continue. */
      if ( tcp == NIL(CELL) ) {
	 FREE(dp);
	 if ( prev == NIL(LINK) ) {
	    cp->ce_prq = next;
	    dp = NULL;		/* dp will be the new value of prev. */
	 }
	 else {
	    prev->cl_next = next;
	    dp = prev;
	 }
	 continue;
      }

      /* Clear F_MARK flag that could have been set by _expand_dynamic_prq(). */ 
      tcp->ce_flag &= ~(F_MARK);

      if( cp->ce_attr & A_LIBRARY ) {
         tcp->ce_attr |= A_LIBRARYM;
	 tcp->ce_lib   = cp->ce_fname;
      }

      /* Propagate the parent's F_REMOVE and F_INFER flags to the
       * prerequisites. */
      tcp->ce_flag |= cp->ce_flag & (F_REMOVE|F_INFER);

      /* Propagate parents A_ROOT attribute to a child if the parent is a
       * F_MULTI target. */
      if( (cp->ce_flag & F_MULTI) && (cp->ce_attr & A_ROOT) )
	 tcp->ce_attr |= A_ROOT;

      tcp->ce_parent = cp;
      rval |= Make(tcp, setdirroot);

      if( cp->ce_attr & A_LIBRARY )
         tcp->ce_attr ^= A_LIBRARYM;

      /* Return on error or if Make() is still running and A_SEQ is set.
       * (All F_MULTI targets have the A_SEQ attribute.)  */
      if( rval == -1 || (seq && (rval==1)) )
	 goto stop_making_it;

      /* If tcp is ready, set made = F_MADE. */
      made &= tcp->ce_flag & F_MADE;
   }


   /* Do the loop again.  We are most definitely going to make the current
    * cell now.  NOTE:  doing this loop here also results in a reduction
    * in peak memory usage by the algorithm. */

   for( dp = cp->ce_prq; dp != NIL(LINK); dp = dp->cl_next ) {
      int  tgflg;
      tcp  = dp->cl_prq;
      if( tcp == NIL(CELL) )
	 Fatal("Internal Error: Found prerequisite list cell without prerequisite!");

      name = tcp->ce_fname;

      /* make certain that all prerequisites are made prior to advancing. */
      if( !(tcp->ce_flag & F_MADE) ) goto stop_making_it;

      /* If the target is a library, then check to make certain that a member
       * is newer than an object file sitting on disk.  If the disk version
       * is newer then set the time stamps so that the archived member is
       * replaced. */
      if( cp->ce_attr & A_LIBRARY )
	 if( tcp->ce_time <= cp->ce_time ) {
	    time_t mtime = Do_stat( name, tcp->ce_lib, NIL(char *), FALSE );
	    if( mtime < tcp->ce_time ) tcp->ce_time = cp->ce_time+1L;
	 }

      /* Set otime to the newest time stamp of all prereqs or 1 if there
       * are no prerequisites. */
      if( tcp->ce_time > otime ) otime = tcp->ce_time;

      list_add(&all_list, name);
      if( (tgflg = (dp->cl_flag & F_TARGET)) != 0 )
         list_add(&inf_list, name);

      if((cp->ce_time<tcp->ce_time) || ((tcp->ce_flag & F_TARGET) && Force)) {
         list_add(&outall_list, name);
         if( tgflg )
            list_add(&imm_list, name);
      }
   }

   /* If we are building a F_MULTI target inherit the time from
    * its children. */
   if( (cp->ce_flag & F_MULTI) )
      cp->ce_time = otime;

   /* All prerequisites are made, now make the current target. */

   /* Restore UseWinpath and $@ if needed, see above for an explanation. */
   if (m_at->ht_value == NIL(char)) {
      /* This check effectively tests if Make() was run before because
       * Make() frees all dynamic macro values at the end. */
#if defined(__CYGWIN__)
      UseWinpath = (((cp->ce_attr|Glob_attr)&A_WINPATH) != 0);
#endif
      m_at = Def_macro("@", DO_WINPATH(cp->ce_fname), M_MULTI);
   }
 
   /* Create a string with all concatenate filenames. The function
    * respects .WINPATH.  Note that gen_path_list_string empties its
    * parameter :( */
   all = gen_path_list_string(&all_list);
   imm = gen_path_list_string(&imm_list);
   outall = gen_path_list_string(&outall_list);
   inf = gen_path_list_string(&inf_list);
   
   DB_PRINT( "mem", ("%s:-C mem %ld", cp->CE_NAME, (long) coreleft()) );
   DB_PRINT( "make", ("I make '%s' if %ld > %ld", cp->CE_NAME, otime,
	      cp->ce_time) );

   if( Verbose & V_MAKE ) {
      printf( "%s:  >>>> Making ", Pname );
      /* Also print the F_MULTI master target. */
      if( cp->ce_flag & F_MULTI )
	 printf( "(::-\"master\" target) " );
      if( cp->ce_count != 0 )
	 printf( "[%s::{%d}]\n", cp->CE_NAME, cp->ce_count );
      else
	 printf( "[%s]\n", cp->CE_NAME );
   }


   /* Only PWD, TMD, MAKEDIR and the dynamic macros are affected by
    * .WINPATH. $@ is handled earlier, do the rest now. */
#if defined(__CYGWIN__)
   /* This is only relevant for cygwin. */
   if( UseWinpath != prev_winpath_attr ) {
      Def_macro( "MAKEDIR", Makedir, M_FORCE | M_EXPANDED );
      /* If push is TRUE (Push_dir() was used) PWD and TMD are already
       * set. */
      if( !push ) {
	 Def_macro( "PWD", Pwd, M_FORCE | M_EXPANDED );
	 _set_tmd();
      }
   }
   prev_winpath_attr = UseWinpath;
#endif

   /* Set the remaining dynamic macros $*, $>, $?, $<, $& and $^. */

   /* $* is either expanded as the result of a % inference or defined to
    * $(@:db) and hence unexpanded otherwise. The latter doesn't start
    * with / and will therefore not be touched by DO_WINPATH(). */
   m_bb = Def_macro( "*", DO_WINPATH(cp->ce_per),   M_MULTI );

   /* This is expanded. */
   m_g  = Def_macro( ">", DO_WINPATH(cp->ce_lib),   M_MULTI|M_EXPANDED );
   /* These strings are generated with gen_path_list_string() and honor
    * .WINPATH */
   m_q  = Def_macro( "?", outall,       M_MULTI|M_EXPANDED );
   m_b  = Def_macro( "<", inf,          M_MULTI|M_EXPANDED );
   m_l  = Def_macro( "&", all,          M_MULTI|M_EXPANDED );
   m_up = Def_macro( "^", imm,          M_MULTI|M_EXPANDED );

   _recipes[ RP_RECIPE ] = cp->ce_recipe;

   /* We attempt to make the target if
    *   1. it has a newer prerequisite
    *   2. It is a target and Force is set
    *   3. It's set of recipe lines has changed.
    */
   if(   Check_state(cp, _recipes, NUM_RECIPES )
      || (cp->ce_time < otime)
      || ((cp->ce_flag & F_TARGET) && Force)
     ) {

      if( Measure & M_TARGET )
	 Do_profile_output( "s", M_TARGET, cp );

      /* Only checking so stop as soon as we determine we will make
       * something */
      if( Check ) {
	 rval = -1;
	 goto stop_making_it;
      }

      if( Verbose & V_MAKE )
	 printf( "%s:  Updating [%s], (%ld > %ld)\n", Pname,
		 cp->CE_NAME, otime, cp->ce_time );

      /* In order to check if a targets time stamp was properly updated
       * after the target was made and to keep the dependency chain valid
       * for targets without recipes we store the minimum required file
       * time. If the target time stamp is older than the newest
       * prerequisite use that time, otherwise the current time. (This
       * avoids the call to Do_time() for every target, still checks
       * if the target time is new enough for the given prerequisite and
       * mintime is also the newest time of the given prerequisites and
       * can be used for targets without recipes.)
       * We reuse the ce_time member to store this minimum time until
       * the target is finished by Update_time_stamp(). This function
       * checks if the file time was updated properly and warns if it was
       * not. (While making a target this value does not change.) */
      cp->ce_time = ( cp->ce_time < otime ? otime : Do_time() );
      DB_PRINT( "make", ("Set ce_time (mintime) to: %ld", cp->ce_time) );

      if( Touch ) {
	 name = cp->ce_fname;
	 lib  = cp->ce_lib;

	 if( (!(Glob_attr & A_SILENT) || !Trace) && !(cp->ce_attr & A_PHONY) ) {
	    if( lib == NIL(char) )
	       printf("touch(%s)", name );
	    else if( cp->ce_attr & A_SYMBOL )
	       printf("touch(%s((%s)))", lib, name );
	    else
	       printf("touch(%s(%s))", lib, name );
	 }

	 if( !Trace && !(cp->ce_attr & A_PHONY) )
	    if( Do_touch( name, lib,
		(cp->ce_attr & A_SYMBOL) ? &name : NIL(char *) ) != 0 )
	       printf( "  not touched - non-existant" );

	 if( (!(Glob_attr & A_SILENT) || !Trace) && !(cp->ce_attr & A_PHONY) )
	    printf( "\n" );

	 Update_time_stamp( cp );
      }
      else if( cp->ce_recipe != NIL(STRING) ) {
	 /* If a recipe is found use it. Note this misses F_MULTI targets. */
	 if( !(cp->ce_flag & F_SINGLE) ) /* Execute the recipes once ... */
	       rval = Exec_commands( cp );
	 /* Update_time_stamp() is called inside Exec_commands() after the
	  * last recipe line is finished. (In _finished_child()) */
	 else {				 /* or for every out of date dependency
					  * if the ruleop ! was used. */
	    TKSTR tk;

	    /* We will redefine $? to be the prerequisite that the recipes
	     * are currently evaluated for. */
	    _drop_mac( m_q );

	    /* Execute recipes for each out out of date prerequisites.
	     * WARNING! If no prerequisite is given the recipes are not
	     * executed at all! */
	    if( outall && *outall ) {
	       /* Wait for each prerequisite to finish, save the status
		* of Wait_for_completion. */
	       int wait_for_completion_status = Wait_for_completion;
	       Wait_for_completion = TRUE;

	       SET_TOKEN( &tk, outall );

	       /* No need to update the target timestamp/removing temporary
		* prerequisites (Update_time_stamp() in _finished_child())
		* until all prerequisites are done. */
	       Doing_bang = TRUE;
	       name = Get_token( &tk, "", FALSE );
	       /* This loop might fail if outall contains filenames with
		* spaces. */
	       do {
		  /* Set $? to current prerequisite. */
		  m_q->ht_value = name;

		  rval = Exec_commands( cp );
		  /* Thanks to Wait_for_completion = TRUE we are allowed
		   * to remove the temp files here. */
		  Unlink_temp_files(cp);
	       }
	       while( *(name = Get_token( &tk, "", FALSE )) != '\0' );
	       Wait_for_completion = wait_for_completion_status;
	       Doing_bang = FALSE;
	    }

	    Update_time_stamp( cp );
	    /* Erase $? again. Don't free the pointer, it was part of outall. */
	    m_q->ht_value = NIL(char);
	 }
      }
      else if( !(cp->ce_flag & F_RULES) && !(cp->ce_flag & F_STAT) &&
	       (!(cp->ce_attr & A_ROOT) || !(cp->ce_flag & F_EXPLICIT)) &&
	       !(cp->ce_count) )
	 /* F_MULTI subtargets should evaluate its parents F_RULES value
	  * but _make_multi always sets the F_RULES value of the master
	  * target. Assume F_RULES is set for subtargets. This might not
	  * be true if there are no prerequisites and no recipes in any
	  * of the subtargets. (FIXME) */
	 Fatal( "Don't know how to make `%s'",cp->CE_NAME );
      else {
         /* Empty recipe, set the flag as MADE and update the time stamp */
         /* This might be a the master cell of a F_MULTI target. */
	 Update_time_stamp( cp );
      }
   }
   else {
      if( Verbose & V_MAKE )
	 printf( "%s:  Up to date [%s], prq time = %ld , target time = %ld)\n", Pname,
		 cp->CE_NAME, otime, cp->ce_time );
      mark_made = TRUE;
   }

   /* If mark_made == TRUE the target is up-to-date otherwise it is
    * currently in the making. */

   /* Update all targets in .UPDATEALL rule / only target cp. */
   for(dp=CeMeToo(cp); dp; dp=dp->cl_next) {
      tcp=dp->cl_prq;

      /* Set the time stamp of those prerequisites without rule to the current
       * time if Force is TRUE to make sure that targets depending on those
       * prerequisites get remade. */
      if( !(tcp->ce_flag & F_TARGET) && Force ) tcp->ce_time = Do_time();
      if( mark_made ) {
	 tcp->ce_flag |= F_MADE;
	 if( tcp->ce_flag & F_MULTI ) {
	    LINKPTR tdp;
	    for( tdp = tcp->ce_prq; tdp != NIL(LINK); tdp = tdp->cl_next )
	       tcp->ce_attr |= tdp->cl_prq->ce_attr & A_UPDATED;
	 }
      }

      /* Note that the target is in the making. */
      tcp->ce_flag |= F_VISITED;

      /* Note:  If the prerequisite was made using a .SETDIR= attribute
       * 	directory then we will include the directory in the fname
       *        of the target.  */
      if( push ) {
	 char *dir   = nsetdirroot ? nsetdirroot->ce_dir : Makedir;
	 /* get relative path from current SETDIR to new SETDIR. */
	 /* Attention, even with .WINPATH set this has to be a POSIX
	  * path as ce_fname neeed to be POSIX. */
	 char *pref  = _prefix( dir, tcp->ce_dir );
	 char *nname = Build_path(pref, tcp->ce_fname);

	 FREE(pref);
	 if( (tcp->ce_attr & A_FFNAME) && (tcp->ce_fname != NIL(char)) )
	    FREE( tcp->ce_fname );

	 tcp->ce_fname = DmStrDup(nname);
	 tcp->ce_attr |= A_FFNAME;
      }
   }

stop_making_it:
   _drop_mac( m_g  );
   _drop_mac( m_q  );
   _drop_mac( m_b  );
   _drop_mac( m_l  );
   _drop_mac( m_bb );
   _drop_mac( m_up );
   _drop_mac( m_at );

   /* undefine conditional macros if any */
   for(dp=CeMeToo(cp); dp; dp=dp->cl_next) {
      tcp=dp->cl_prq;

      while (tcp->ce_pushed != NIL(HASH)) {
	 HASHPTR cur = tcp->ce_pushed;
	 tcp->ce_pushed = cur->ht_link;

	 Pop_macro(cur);
	 FREE(cur->ht_name);
	 if(cur->ht_value)
	    FREE(cur->ht_value);
	 FREE(cur);
      }
   }

   if( push )
      Pop_dir(FALSE);

   /* Undefine the strings that we used for constructing inferred
    * prerequisites. */
   if( inf    != NIL(char) ) FREE( inf    );
   if( all    != NIL(char) ) FREE( all    );
   if( imm    != NIL(char) ) FREE( imm    );
   if( outall != NIL(char) ) FREE( outall );
   free_list(all_list.first);
   free_list(imm_list.first);
   free_list(outall_list.first);
   free_list(inf_list.first);

   DB_PRINT( "mem", ("%s:-< mem %ld", cp->CE_NAME, (long) coreleft()) );
   DB_RETURN(rval);
}


static char *
_prefix( pfx, pat )/*
=====================
   Return the relative path from pfx to pat. Both paths have to be absolute
   paths. If the paths are on different resources or drives (if applicable)
   or only share a relative path going up to the root dir and down again
   return pat. */
char *pfx;
char *pat;
{
   char *cmp1=pfx;
   char *cmp2=pat;
   char *tpat=pat; /* Keep pointer to original pat. */
   char *result;
   char *up;
   int first = 1;
   int samerootdir = 1; /* Marks special treatment for the root dir. */
#ifdef HAVE_DRIVE_LETTERS
   int pfxdl = 0;
   int patdl = 0;
#endif

   /* Micro optimization return immediately if pfx and pat are equal. */
   if( strcmp(pfx, pat) == 0 )
      return(DmStrDup(""));

#ifdef HAVE_DRIVE_LETTERS
   /* remove the drive letters to avoid getting them into the relative
    * path later. */
   if( *pfx && pfx[1] == ':' && isalpha(*pfx) ) {
      pfxdl = 1;
      cmp1 = DmStrSpn(pfx+2, DirBrkStr);
   }
   if( *pat && pat[1] == ':' && isalpha(*pat) ) {
      patdl = 1;
      cmp2 = DmStrSpn(pat+2, DirBrkStr);
   }
   /* If the drive letters are different use the abs. path. */
   if( pfxdl && patdl && (tolower(*pfx) != tolower(*pat)) )
      return(DmStrDup(pat));

   /* If only one has a drive letter also use the abs. path. */
   if( pfxdl != patdl )
      return(DmStrDup(pat));
   else if( pfxdl )
      /* If both are the same drive letter, disable the special top
       * dir treatment. */
      samerootdir = 0;

   /* Continue without the drive letters. (Either none was present,
    * or both were the same. This also solves the problem that the
    * case of the drive letters sometimes depends on the shell.
    * (cmd.exe vs. cygwin bash) */
   pfx = cmp1;
   pat = cmp2;
#endif

   /* Cut off equal leading parts of pfx, pat. Both have to be abs. paths. */
   while(*pfx && *pat) {
      /* skip leading dir. separators. */
      pfx = DmStrSpn(cmp1, DirBrkStr);
      pat = DmStrSpn(cmp2, DirBrkStr);

      /* Only check in the first run of the loop. Leading slashes can only
       * mean POSIX paths or Windows resources (two) slashes. Drive letters
       * have no leading slash. In any case, if the number of slashes are
       * not equal there can be no relative path from one two the other.
       * In this case return the absolute path. */
      if( first ) {
	 if( cmp1-pfx != cmp2-pat ) {
	    return(DmStrDup(tpat));
	 }
	 first = 0;
      }

      /* find next dir. separator (or ""). */
      cmp1 = DmStrPbrk(pfx, DirBrkStr);
      cmp2 = DmStrPbrk(pat, DirBrkStr);

      /* if length of directory name is equal compare the strings. If equal
       * go into next loop. If not equal and directory names in the root
       * dir are compared return the absolut path otherwise break the loop
       * and construct the relative path from pfx to pat. */
      if ( (cmp1-pfx) != (cmp2-pat) || strncmp(pfx,pat,cmp1-pfx) != 0 ) {
	 if( samerootdir ) {
	    return(DmStrDup(tpat));
	 }
	 break;
      }

      if( samerootdir ) {
#if __CYGWIN__
	 /* If the toplevel directory is /cygdrive (or the equivalent prefix)
	  * treat the following level also as rootdir. If we are here cmp1-pfx
	  * cannot be zero so we won't compare with an empty cygdrive prefix. */
	 if ( (cmp1-pfx) == CygDrvPreLen && strncmp(pfx,CygDrvPre,CygDrvPreLen) == 0 )
	    samerootdir = 1;
	 else
#endif
	    samerootdir = 0;
      }
   }

   result = DmStrDup("");
   up = DmStrJoin("..",DirSepStr,-1,FALSE);
   cmp1 = pfx;
   /* Add "../" for each directory in pfx */
   while ( *(pfx=DmStrSpn(cmp1,DirBrkStr)) != '\0' ) {
      cmp1 = DmStrPbrk(pfx,DirBrkStr);
      result = DmStrJoin(result,up,-1,TRUE);
   }
   FREE(up);

   pat = DmStrSpn(pat,DirBrkStr);
   /* Append pat to result. */
   if( *pat != '\0' ) {
      cmp2 = DmStrDup(Build_path(result, pat));
      FREE(result);
      result = cmp2;
   } else {
      /* if pat is empty and result exists remove the trailing slash
       * from the last "../". */
      if( *result ) {
	 result[strlen(result)-1] = '\0';
      }
   }

   return(result);
}


static LINKPTR
_dup_prq( lp )
LINKPTR lp;
{
   LINKPTR tlp;

   if( lp == NIL(LINK) ) return(lp);

   TALLOC(tlp, 1, LINK);
   tlp->cl_prq  = lp->cl_prq;
   tlp->cl_flag = lp->cl_flag;
   tlp->cl_next = _dup_prq( lp->cl_next );

   return(tlp);
}


static LINKPTR
_expand_dynamic_prq( head, lp, name )/*
=======================================
   The string name can contain one or more target names. Check if these are
   already a prerequisite for the current target. If not add them to the list
   of prerequisites. If no prerequisites were added set lp->cl_prq to NULL.
   Set the F_MARK flag to indicate that the prerequisite was expanded.
   Use cl_flag instead?? */
LINKPTR head;
LINKPTR lp;
char *name;
{
   CELLPTR cur = lp->cl_prq;

   if( !(*name) ) {
      /* If name is empty this leaves lp->cl_prq unchanged -> No prerequisite added. */
      ;
   }
   else if ( strchr(name, ' ') == NIL(char) ) {
      /* If condition above is true, no space is found. */
      CELLPTR prq  = Def_cell(name);
      LINKPTR tmp;

      /* Check if prq already exists. */
      for(tmp=head;tmp != NIL(LINK) && tmp->cl_prq != prq;tmp=tmp->cl_next);

      /* If tmp is NULL then the prerequisite is new and is added to the list. */
      if ( !tmp ) {
	 /* replace the prerequisite with the expanded version. */
	 lp->cl_prq = prq;
	 lp->cl_prq->ce_flag |= F_MARK;
      }
   }
   else {
      LINKPTR tlp  = lp;
      LINKPTR next = lp->cl_next;
      TKSTR token;
      char  *p;
      int   first=TRUE;

      /* Handle more than one prerequisite. */
      SET_TOKEN(&token, name);
      while (*(p=Get_token(&token, "", FALSE)) != '\0') {
	 CELLPTR prq = Def_cell(p);
	 LINKPTR tmp;

	 for(tmp=head;tmp != NIL(LINK) && tmp->cl_prq != prq;tmp=tmp->cl_next);
	 
	 /* If tmp is not NULL the prerequisite already exists. */
	 if ( tmp ) continue;

	 /* Add list elements behind the first if more then one new
	  * prerequisite is found. */
	 if ( first ) {
	    first = FALSE;
	 }
	 else {
	    TALLOC(tlp->cl_next,1,LINK);
	    tlp = tlp->cl_next;
	    tlp->cl_flag |= F_TARGET;
	    tlp->cl_next = next;
	 }

	 tlp->cl_prq = prq;
	 tlp->cl_prq->ce_flag |= F_MARK;
      }
      CLEAR_TOKEN( &token );
   }

   /* If the condition is true no new prerequisits were found. */
   if ( lp->cl_prq == cur ) {
      lp->cl_prq = NIL(CELL);
      lp->cl_flag = 0;
   }

   /* Is returned unchanged. */
   return(lp);
}


static void
_drop_mac( hp )/*
================ set a macro value to zero. */
HASHPTR hp;
{
   if( hp ) {
      char * value = hp->ht_value;
      if( value != NIL(char) ) {
         hp->ht_value = NIL(char);
         FREE( value );
      }
   }
}



static int
_explode_graph( cp, parent, setdirroot )/*
==========================================
   Check to see if we have made the node already.  If so then don't do
   it again, except if the cell's ce_setdir field is set to something other
   than the value of setdirroot.  If they differ then, and we have made it
   already, then make it again and set the cell's stat bit to off so that
   we do the stat again.  */
CELLPTR cp;
LINKPTR parent;
CELLPTR setdirroot;
{
   static CELLPTR removecell = NIL(CELL);
   
   if ( removecell == NIL(CELL) ) 
      removecell = Def_cell(".REMOVE");

   /* we may return if we made it already from the same setdir location,
    * or if it is not a library member whose lib field is non NULL.  (if
    * it is such a member then we have a line of the form:
    *	lib1 lib2 .LIBRARY : member_list...
    * and we have to make sure all members are up to date in both libs. */

   if ( setdirroot == removecell )
      return( 0 );

   if( cp->ce_setdir == setdirroot &&
       !((cp->ce_attr & A_LIBRARYM) && (cp->ce_lib != NIL(char))) )
      return( 0 );

   /* We check to make sure that we are comming from a truly different
    * directory, ie. ".SETDIR=joe : a.c b.c d.c" are all assumed to come
    * from the same directory, even though setdirroot is different when
    * making dependents of each of these targets. */

   if( cp->ce_setdir != NIL(CELL) &&
       setdirroot != NIL(CELL) &&
       cp->ce_dir &&
       setdirroot->ce_dir &&
       !strcmp(cp->ce_dir, setdirroot->ce_dir) )
      return( 0 );

   if( Max_proc > 1 ) {
      LINKPTR dp;

      TALLOC(parent->cl_prq, 1, CELL);
      *parent->cl_prq = *cp;
      cp = parent->cl_prq;
      cp->ce_prq = _dup_prq(cp->ce_prqorg);
      cp->ce_all.cl_prq = cp;
      CeNotMe(cp) = _dup_prq(CeNotMe(cp));

      for(dp=CeNotMe(cp);dp;dp=dp->cl_next) {
	 CELLPTR tcp = dp->cl_prq;
	 TALLOC(dp->cl_prq,1,CELL);
	 *dp->cl_prq = *tcp;
	 dp->cl_prq->ce_flag &= ~(F_STAT|F_VISITED|F_MADE);
	 dp->cl_prq->ce_set   = cp;
      }      
   }
   cp->ce_flag  &= ~(F_STAT|F_VISITED|F_MADE);

   /* Indicate that we exploded the graph and that the current node should
    * be made. */
   return(1);
}



PUBLIC int
Exec_commands( cp )/*
=====================
  Execute the commands one at a time that are pointed to by the rules pointer
  of the target cp if normal (non-group) recipes are defined. If a group recipe
  is found all commands are written into a temporary file first and this
  (group-) shell script is executed all at once.
  If a group is indicated, then the ce_attr determines .IGNORE and .SILENT
  treatment for the group.

  The function returns 0, if the command is executed and has successfully
  returned, and it returns 1 if the command is executing but has not yet
  returned or -1 if an error occurred (Return value from Do_cmnd()).

  Macros that are found in recipe lines are expanded in this function, in
  parallel builds this can mean they are expanded before the previous recipe
  lines are finished. (Exception: $(shell ..) waits until all previous recipe
  lines are done.)

  The F_MADE bit in the cell is guaranteed set when the command has
  successfully completed.  */
CELLPTR cp;
{
   static HASHPTR useshell = NIL(HASH);
   static HASHPTR command  = NIL(HASH);
   static         int   read_cmnd = 0;
   register STRINGPTR	rp;
   STRINGPTR            orp;
   char			*cmnd;
   char			*groupfile;
   FILE    		*tmpfile = 0;
   int			do_it;
   t_attr		attr;
   int			group;
   int			trace;
   int			rval  = 0;

   DB_ENTER( "Exec_commands" );

   if( cp->ce_recipe == NIL(STRING) )
      Fatal("Internal Error: No recipe found!");

   attr  = Glob_attr | cp->ce_attr;
   trace = Trace || !(attr & A_SILENT);
   group = cp->ce_flag & F_GROUP;

   /* Do it again here for those that call us from places other than Make()
    * above. */
   orp = _recipes[ RP_RECIPE ];
   _recipes[ RP_RECIPE ] = cp->ce_recipe;

   if( group ) {
      /* Leave this assignment of Current_target here.  It is needed just
       * incase the user hits ^C after the tempfile for the group recipe
       * has been opened. */
      Current_target = cp;
      trace  = Trace || !(attr & A_SILENT);

      if( !Trace ) tmpfile = Start_temp( Grp_suff, cp, &groupfile );
      if( trace )  fputs( "[\n", stdout );

      /* Emit group prolog */
      if( attr & A_PROLOG )
         _append_file( _recipes[RP_GPPROLOG], tmpfile, cp->CE_NAME, trace );
   }

   if( !useshell )
      useshell=Def_macro("USESHELL",NIL(char),M_MULTI|M_EXPANDED);

   if( !read_cmnd ) {
      command = GET_MACRO("COMMAND");
      read_cmnd = 1;
   }

   /* Process commands in recipe. If in group, merely append to file.
    * Otherwise, run them.  */
   for( rp=_recipes[RP_RECIPE]; rp != NIL(STRING); rp=rp->st_next) {
      t_attr a_attr = A_DEFAULT;
      t_attr l_attr;
      char   *p;
      int    new_attr = FALSE;
      int    shell; /* True if the recipe shall run in shell. */

      /* Reset it for each recipe line otherwise tempfiles don't get removed.
       * Since processing of $(mktmp ...) depends on Current_target being
       * correctly set. */
      Current_target = cp;

      /* Only check for +,-,%,@ if the recipe line begins with a '$' macro
       * expansion.  Otherwise there is no way it is going to find these
       * now. */
      if( *rp->st_string == '$' && !group ) {
         t_attr s_attr = Glob_attr;
	 Glob_attr |= A_SILENT;
	 Suppress_temp_file = TRUE;
	 cmnd = Expand(rp->st_string);
	 Suppress_temp_file = FALSE;
	 a_attr |= Rcp_attribute(cmnd);
	 FREE(cmnd);
	 ++new_attr;
	 Glob_attr = s_attr;
      }

      l_attr = attr|a_attr|rp->st_attr;
      shell  = ((l_attr & A_SHELL) != 0);
      useshell->ht_value = (group||shell)?"yes":"no";

      /* All macros are expanded before putting them in the "process queue".
       * Nothing in Expand() should be able to change dynamic macros. */
      cmnd = Expand( rp->st_string );

      if( new_attr && (p = DmStrSpn(cmnd," \t\n+-@%")) != cmnd ) {
	 size_t len = strlen(p)+1;
	 memmove(cmnd,p,len);
      }

      /* COMMAND macro is set to "$(CMNDNAME) $(CMNDARGS)" by default, it is
       * possible for the user to reset it to, for example
       *	COMMAND = $(CMNDNAME) @$(mktmp $(CMNDARGS))
       * in order to get a different interface for his command execution. */
      if( command != NIL(HASH) && !group ) {
	 char *cname = cmnd;
         char cmndbuf[30];

	 if ( *(p=DmStrPbrk(cmnd," \t\n")) != '\0' ) {
	    *p = '\0';
	    (void)Def_macro("CMNDARGS",DmStrSpn(p+1," \t\n"),M_MULTI|M_EXPANDED);
	 }
	 else
	    (void) Def_macro("CMNDARGS","",M_MULTI|M_EXPANDED);

	 (void) Def_macro("CMNDNAME",cname,M_MULTI|M_EXPANDED);

         strcpy(cmndbuf, "$(COMMAND)");
	 cmnd = Expand(cmndbuf);
	 FREE(cname); 			/* cname == cmnd at this point. */

	 /* Collect up any new attributes */
	 l_attr |= Rcp_attribute(cmnd);
	 shell  = ((l_attr & A_SHELL) != 0);

	 /* clean up the attributes that we may have just added. */
	 if( (p = DmStrSpn(cmnd," \t\n+-@%")) != cmnd ) {
	    size_t len = strlen(p)+1;
	    memmove(cmnd,p,len);
	 }
      }

#if defined(MSDOS) && defined(REAL_MSDOS)
      Swap_on_exec = ((l_attr & A_SWAP) != 0);	  /* Swapping for DOS only */
#endif
      do_it = !Trace;

      /* We force execution of the recipe if we are tracing and the .EXECUTE
       * attribute was given or if the it is not a group recipe and the
       * recipe line contains the string $(MAKE). Wait_for_completion might
       * be changed gobaly but this is without consequences as we wait for
       * every recipe with .EXECUTE and don't start anything else. */
      if( Trace
       && ((l_attr & A_EXECUTE)||(!group && DmStrStr(rp->st_string,"$(MAKE)")))
      ) {
	 Wait_for_completion |= Trace;
	 do_it = TRUE;
      }

      if( group )
	 /* Append_line() calls Print_cmnd(). */
         Append_line( cmnd, TRUE, tmpfile, cp->CE_NAME, trace, 0 );
      else {
	 /* Don't print empty recipe lines. .ROOT and .TARGETS
	  * deliberately might have empty "" recipes and we don't want
	  * to output empty recipe lines for them. */
	 if ( *cmnd ) {
	    /* Print command and remove continuation sequence from cmnd. */
	    Print_cmnd(cmnd, !(do_it && (l_attr & A_SILENT)), 0);
	 }
	 rval=Do_cmnd(&cmnd,FALSE,do_it,cp,l_attr,
		      rp->st_next == NIL(STRING) );
      }

      FREE(cmnd);
   }

   /* If it is a group then output the EPILOG if required and possibly
    * execute the command */
   if( group && !(cp->ce_attr & A_ERROR) ) { 
      if( attr & A_EPILOG )	/* emit epilog */
	 _append_file( _recipes[RP_GPEPILOG], tmpfile, cp->CE_NAME, trace );

      if( trace ) fputs("]\n", stdout);

      do_it = !Trace;
      if( do_it )
	{
		Close_temp( cp, tmpfile );
#if defined(UNIX)

 		chmod(groupfile,0700);
#endif
	} 			
      rval = Do_cmnd(&groupfile, TRUE, do_it, cp, attr | A_SHELL, TRUE);
   }

   _recipes[ RP_RECIPE ] = orp;
   cp->ce_attr &= ~A_ERROR;
   DB_RETURN( rval );
}


PUBLIC void
Print_cmnd( cmnd, echo, map )/*
================================
   This routine is called to print out the command to stdout.  If echo is
   false the printing to stdout is supressed.
   The routine is also used to remove the line continuation sequence
   \<nl> from the command string and convert escape sequences if the
   map flag is set.
   The changed string is used later to actually to execute the command. */
char *cmnd;
int  echo;
int  map;
{
   register char *p;
   register char *n;
   char tmp[3];

   DB_ENTER( "Print_cmnd" );
   
   if( echo ) {
      printf( "%s\n", cmnd  );
      fflush(stdout);
   }

   tmp[0] = ESCAPE_CHAR;
   tmp[1] = CONTINUATION_CHAR;
   tmp[2] = '\0';

   for( p=cmnd; *(n = DmStrPbrk(p,tmp)) != '\0'; )
      /* Remove the \<nl> sequences. */
      if(*n == CONTINUATION_CHAR && n[1] == '\n') {
	 size_t len = strlen(n+2)+1;
	 DB_PRINT( "make", ("fixing [%s]", p) );
	 memmove( n, n+2, len );
	 p = n;
      }
      /* Look for an escape sequence and replace it by it's corresponding
       * character value. */
      else {
         if( *n == ESCAPE_CHAR && map ) Map_esc( n );
	 p = n+1;
      }

   DB_VOID_RETURN;
}



/* These routines are used to maintain a stack of directories when making
 * the targets.  If a target cd's to the directory then it is assumed that
 * it will undo it when it is finished making itself. */

static STRINGPTR dir_stack = NIL(STRING);

PUBLIC int
Push_dir( dir, name, ignore )/*
===============================
   Change the current working directory to dir and save the current
   working directory on the stack so that we can come back.
   
   If ignore is TRUE then do not complain about _ch_dir if not possible.

   Return 1 if the directory change was successfull and 0 otherwise. */
char *dir;
char *name;
int  ignore;
{
   STRINGPTR   new_dir;

   DB_ENTER( "Push_dir" );

   if( dir == NIL(char)  || *dir == '\0' ) dir = Pwd;
   if( *dir == '\'' && dir[strlen(dir)-1] == '\'' ) {
      dir = DmStrDup(dir+1);
      dir[strlen(dir)-1]='\0';
   }
   else if (strchr(dir,'$') != NIL(char))
      dir = Expand(dir);
   else
      dir = DmStrDup(dir);

   if( Set_dir(dir) ) {
      if( !ignore )
         Fatal( "Unable to change to directory `%s', target is [%s]",
	        dir, name );
      FREE(dir);
      DB_RETURN( 0 );
   }

   DB_PRINT( "dir", ("Push: [%s]", dir) );
   if( Verbose & V_DIR_SET )
      printf( "%s:  Changed to directory [%s]\n", Pname, dir  );

   FREE( dir );
   TALLOC( new_dir, 1, STRING );
   new_dir->st_next   = dir_stack;
   dir_stack          = new_dir;
   new_dir->st_string = DmStrDup( Pwd );

   Def_macro( "PWD", Get_current_dir(), M_FORCE | M_EXPANDED );
   _set_tmd();

   DB_RETURN( 1 );
}



PUBLIC void
Pop_dir(ignore)/*
=================
   Change the current working directory to the previous saved dir. */
int ignore;
{
   STRINGPTR old_dir;
   char      *dir;

   DB_ENTER( "Pop_dir" );

   if( dir_stack == NIL(STRING) ) {
      if( ignore ) {
         DB_VOID_RETURN;
      }
      else
	 Error( "Directory stack empty for return from .SETDIR" );
   }

   if( Set_dir(dir = dir_stack->st_string) )
      Fatal( "Could not change to directory `%s'", dir );

   Def_macro( "PWD", dir, M_FORCE | M_EXPANDED );
   DB_PRINT( "dir", ("Pop: [%s]", dir) );
   if( Verbose & V_DIR_SET )
      printf( "%s:  Changed back to directory [%s]\n", Pname, dir);

   old_dir   = dir_stack;
   dir_stack = dir_stack->st_next;

   FREE( old_dir->st_string );
   FREE( old_dir );
   _set_tmd();

   DB_VOID_RETURN;
}



static void
_set_tmd()/*
============
   Set the TMD Macro and the Tmd global variable. TMD stands for "To MakeDir"
   and is the path from the present directory (value of $(PWD)) to the directory
   dmake was started up in (value of $(MAKEDIR)). As _prefix() can return absolute
   paths some special .WINPATH treatment is needed.
*/
{
   char  *tmd;

   if( Tmd )
      FREE(Tmd);

   tmd = _prefix(Pwd, Makedir);
   if( *tmd ) {
      Def_macro( "TMD", DO_WINPATH(tmd), M_FORCE | M_EXPANDED );
      Tmd = DmStrDup(tmd);
   } else {
      Def_macro( "TMD", ".", M_FORCE | M_EXPANDED );
      Tmd = DmStrDup(".");
   }
   FREE( tmd );
}


static void
_set_recipe( target, ind )/*
============================
   Set up the _recipes static variable so that the slot passed in points
   at the rules corresponding to the target supplied. */
char *target;
int  ind;
{
   CELLPTR cp;
   HASHPTR hp;

   if( (hp = Get_name(target, Defs, FALSE)) != NIL(HASH) ) {
      cp = hp->CP_OWNR;
      _recipes[ ind ] = cp->ce_recipe;
   }
   else
      _recipes[ ind ] = NIL(STRING);
}



PUBLIC void
Append_line( cmnd, newline, tmpfile, name, printit, map )
char *cmnd;
int  newline;
FILE *tmpfile;
char *name;
int  printit;
int  map;
{
   Print_cmnd( cmnd, printit, map );

   if( Trace ) return;

   fputs(cmnd, tmpfile);
   if( newline ) fputc('\n', tmpfile);
   fflush(tmpfile);

   if( ferror(tmpfile) )
      Fatal("Write error on temporary file, while processing `%s'", name);
}



static void
_append_file( rp, tmpfile, name, printit )
register STRINGPTR rp;
FILE 		   *tmpfile;
char 		   *name;
int 		   printit;
{
   char *cmnd;

   while( rp != NIL(STRING) ) {
      Append_line(cmnd = Expand(rp->st_string), TRUE, tmpfile, name, printit,0);
      FREE(cmnd);
      rp = rp->st_next;
   }
}


#define NUM_BUCKETS	20

typedef struct strpool {
   char 	  *string;	/* a pointer to the string value */
   uint32	  keyval;	/* the strings hash value	 */
   struct strpool *next;	/* hash table link pointer	 */
} POOL, *POOLPTR;

static POOLPTR strings[ NUM_BUCKETS ];

static char *
_pool_lookup( str )/*
=====================
   Scan down the list of chained strings and see if one of them matches
   the string we are looking for. */
char    *str;
{
   register POOLPTR key;
   uint32   keyval;
   uint16   hv;
   uint16   keyindex;
   char     *string;

   DB_ENTER( "_pool_lookup" );

   if( str == NIL(char) ) DB_RETURN("");

   hv  = Hash(str, &keyval);
   key = strings[ keyindex = (hv % NUM_BUCKETS) ];

   while( key != NIL(POOL) )
      if( (key->keyval != keyval) || strcmp(str, key->string) )
	 key = key->next;
      else
	 break;

   if( key == NIL(POOL) ) {
      DB_PRINT( "pool", ("Adding string [%s]", str) );
      TALLOC( key, 1, POOL );			/* not found so add string */
      
      key->string = string = DmStrDup(str);
      key->keyval = keyval;

      key->next           = strings[ keyindex ];
      strings[ keyindex ] = key;
   }
   else {
      DB_PRINT( "pool", ("Found string [%s], key->string") );
      string = key->string;
   }

   DB_RETURN( string );
}


void
Unmake( cp )/*
==============
   Remove flags indicating that a target was previously made.  This
   is used for infered makefiles. */
CELLPTR cp;
{
   LINKPTR dp, ep;
   CELLPTR tcp, pcp;

   DB_ENTER( "Unmake" );

   for(dp=CeMeToo(cp); dp; dp=dp->cl_next) {
      tcp = dp->cl_prq;

      /* Unmake the prerequisites. */
      for( ep = tcp->ce_prq; ep != NIL(LINK); ep = ep->cl_next ) {
	 pcp  = ep->cl_prq;

	 Unmake(pcp);
      }
      DB_PRINT( "unmake", ("Unmake [%s]", tcp->CE_NAME) );

      tcp->ce_flag &= ~(F_MADE|F_VISITED|F_STAT);
      tcp->ce_time  = (time_t)0L;
   }

   DB_VOID_RETURN;
}
