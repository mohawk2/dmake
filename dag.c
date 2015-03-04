/* $RCSfile: dag.c,v $
-- $Revision: 1.12 $
-- last change: $Author: kz $ $Date: 2008-03-05 18:27:48 $
--
-- SYNOPSIS
--      Routines to construct the internal dag.
-- 
-- DESCRIPTION
--	This file contains all the routines that are responsible for
--	defining and manipulating all objects used by the make facility.
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


static void
set_macro_value(hp)/*
=====================
  Set the macro according to its type. In addition to the string value
  in hp->ht_value a macro can stores a value casted with its type.
*/
HASHPTR hp;
{
   switch( hp->ht_flag & M_VAR_MASK )	/* only one var type per var */
   {
      case M_VAR_STRING:
	 *hp->MV_SVAR = hp->ht_value;
	 /* Add special treatment for PWD/MAKEDIR for .WINPATH. */
	 if( hp->MV_SVAR == &Pwd_macval ) {
	    if( Pwd )
	       FREE(Pwd);
	    Pwd = hp->ht_value;
	    /* Use the "DOSified" path for the macro. */
	    *hp->MV_SVAR = hp->ht_value = DmStrDup(DO_WINPATH(hp->ht_value));
	    DB_PRINT( "smv", ("PWD: %s/%s", Pwd_macval, Pwd) );
	 } else if( hp->MV_SVAR == &Makedir_macval ) {
	    if( Makedir )
	       FREE(Makedir);
	    Makedir = hp->ht_value;
	    /* Use the "DOSified" path for the macro. */
	    *hp->MV_SVAR = hp->ht_value = DmStrDup(DO_WINPATH(hp->ht_value));
	    DB_PRINT( "smv", ("MAKEDIR: %s/%s", Makedir_macval, Makedir) );
	 }
	 /* No special treatment for TMD needed. */
	 break;

      case M_VAR_CHAR:
         *hp->MV_CVAR = (hp->ht_value == NIL(char)) ? '\0':*hp->ht_value;
         break;
         
      case M_VAR_INT: {
         int tvalue;
         if( hp->MV_IVAR == NIL(int) ) break;	/* first time */
         
         tvalue = atoi(hp->ht_value);
         if( hp->MV_IVAR == &Buffer_size ) {
	    /* If Buffer_size (MAXLINELENGTH) is modified then make sure
	     * you change the size of the real buffer as well. As the
	     * value will at least be BUFSIZ this might lead to the
	     * situation that the (string) value of MAXLINELENGTH is
	     * smaller than the integer value. */
            tvalue = (tvalue < (BUFSIZ-2)) ? BUFSIZ : tvalue+2;
            if( Buffer_size == tvalue ) break;
            if( Buffer ) FREE(Buffer);
            if((Buffer=MALLOC(tvalue, char)) == NIL(char)) No_ram();
            *Buffer = '\0';
         }
         *hp->MV_IVAR = tvalue;
         
         if( hp->MV_IVAR == &Max_proc || hp->MV_IVAR == &Max_proclmt ) {
            if( tvalue < 1 )
               Fatal( "Process limit value must be > 1" );

#if defined(USE_CREATEPROCESS)
            if( Max_proclmt > MAXIMUM_WAIT_OBJECTS )
               Fatal( "Specified maximum # of processes (MAXPROCESSLIMIT)"
		      " exceeds OS limit of [%d].", MAXIMUM_WAIT_OBJECTS );
#endif
            
            if( Max_proc > Max_proclmt )
               Fatal( "Specified # of processes exceeds limit of [%d]",
                      Max_proclmt );

	    /* Don't change MAXPROCESS value if .SEQUENTIAL is set. */
	    if( (Glob_attr & A_SEQ) && (Max_proc != 1) ) {
	       Warning( "Macro MAXPROCESS set to 1 because .SEQUENTIAL is set." );
	       Max_proc = 1;
	       if( hp->ht_value != NIL(char) ) FREE(hp->ht_value);
	       hp->ht_value = DmStrDup( "1" );
	    }
         }
      } break;
      
      case M_VAR_BIT:
         /* Bit variables are set to 1 if ht_value is not NULL and 0
          * otherwise */
         
         if( hp->ht_value == NIL(char) )
            *hp->MV_BVAR &= ~hp->MV_MASK;
         else {
            *hp->MV_BVAR |= hp->MV_MASK;
	    /* If we're setting .SEQUENTIAL set MAXPROCESS=1. */
	    if( (hp->MV_MASK & A_SEQ) && (Max_proc != 1) )
	       Def_macro( "MAXPROCESS", "1", M_MULTI|M_EXPANDED);
	 }

#if defined(__CYGWIN__)
	 /* Global .WINPATH change. Only needed for cygwin. */
	 if(hp->MV_MASK & A_WINPATH) {
	    UseWinpath = ((Glob_attr&A_WINPATH) != 0);
	    /* Change MAKEDIR, PWD according to .WINPATH. During
	     * makefile evaluation this cannot change TMD (it is "."
	     * and later TMD is set in Make() according to the
	     * .WINPATH attribute. */
	    Def_macro( "MAKEDIR", Makedir, M_FORCE | M_EXPANDED );
	    Def_macro( "PWD", Pwd, M_FORCE | M_EXPANDED );
	 }
#endif
	 break;
   }
}


PUBLIC HASHPTR
Get_name( name, tab, define )/*
===============================
	Look to see if the name is defined, if it is then return
	a pointer to its node, if not return NIL(HASH).
	If define is TRUE and the name is not found it will be added. */

char    *name;			/* name we are looking for   */
HASHPTR *tab;			/* the hash table to look in */
int	define;			/* TRUE => add to table      */
{
   register HASHPTR hp;
   register char    *p;
   uint16           hv;
   uint32           hash_key;

   DB_ENTER( "Get_name" );
   DB_PRINT( "name", ("Looking for %s", name) );

   hp = Search_table( tab, name, &hv, &hash_key );

   if( hp == NIL(HASH) && define ) {
      /* Check to make sure that CELL name contains only printable chars */
      for( p=name; *p; p++ )
	 if( !isprint(*p) && !iswhite(*p) && *p != '\n' )
	    Fatal( "Name contains non-printable character [0x%02x]", *p );

      TALLOC( hp, 1, HASH );		/* allocate a cell and add it in */

      hp->ht_name  = DmStrDup( name );
      hp->ht_hash  = hash_key;
      hp->ht_next  = tab[ hv ];
      tab[ hv ]    = hp;

      DB_PRINT( "name", ("Adding %s", name) );
   }

   DB_PRINT( "name",("Returning: [%s,%lu]",
	     (hp == NIL(HASH)) ? "":hp->ht_name, hv) );
   DB_RETURN( hp );
}


PUBLIC HASHPTR
Search_table( tab, name, phv, phkey )
HASHPTR *tab;
char    *name;
uint16  *phv;
uint32  *phkey;
{
   HASHPTR hp;

   *phv = Hash( name, phkey );

   for( hp = tab[ *phv ]; hp != NIL(HASH); hp = hp->ht_next )
      if(    hp->ht_hash == *phkey
	  && !strcmp(hp->ht_name, name) )
         break;

   return( hp );
}


PUBLIC HASHPTR
Push_macro(hp)/*
================
  This function pushes hp into the hash of all macros. If a previous
  instance of the macro exists it is hidden by the new one. If one
  existed before the new instance inherits some values from the preexisting
  macro (see below for details).
*/
HASHPTR hp;
{
   HASHPTR cur,prev;
   uint16  hv;
   uint32  key;

   hv = Hash(hp->ht_name, &key);

   /* Search for an existing instance of hp->ht_name, if found cur will point
    * to it. */
   for(prev=NIL(HASH),cur=Macs[hv]; cur!=NIL(HASH); prev=cur,cur=cur->ht_next)
      if(    cur->ht_hash == key
	  && !strcmp(cur->ht_name, hp->ht_name) )
         break;

   if (cur == NIL(HASH) || prev == NIL(HASH)) {
      /* If no match or was found or the first element of Macs[hv] was
       * the match insert hp at the beginning. */
      hp->ht_next  = Macs[hv];
      Macs[hv] = hp;
   }
   else {
      /* otherwise insert hp in the chain. */
      hp->ht_next = prev->ht_next;
      prev->ht_next = hp;
   }

   /* Inherit some parts of the former instance. Copying cur->var to hp->var
    * copies the old value. Keeping the M_VAR_MASK (variable type) makes sure
    * the type stays the same keeping M_PRECIOUS assures that the old values
    * cannot be overridden if it is/was set. */
   if (cur) {
      memcpy((void *)&hp->var, (void *)&cur->var, sizeof(hp->var));
      hp->ht_flag |= ((M_VAR_MASK|M_PRECIOUS) & cur->ht_flag);
   }
   
   return(hp);   
}


PUBLIC HASHPTR
Pop_macro(hp)/*
================
  This function pops (removes) hp from the hash of all macros. If a previous
  instance of the macro existed it becomes accessible again.
*/

HASHPTR hp;
{
   HASHPTR cur,prev;
   uint16  hv;
   uint32  key;

   hv = Hash(hp->ht_name, &key);

   /* Try to find hp. */ 
   for(prev=NIL(HASH),cur=Macs[hv]; cur != NIL(HASH);prev=cur,cur=cur->ht_next)
      if (cur == hp)
	 break;

   /* If cur == NIL macros was not found. */
   if (cur == NIL(HASH))
      return(NIL(HASH));

   /* Remove hp from the linked list. */
   if (prev)
      prev->ht_next = cur->ht_next;
   else
      Macs[hv] = cur->ht_next;

   /* Look for a previous (older) instance of hp->ht_name. */
   for(cur=cur->ht_next; cur != NIL(HASH); cur=cur->ht_next)
      if(    cur->ht_hash == key
	  && !strcmp(cur->ht_name, hp->ht_name) )
         break;

   /* If one was found we restore the typecast values. */
   if (cur)
      set_macro_value(cur);

   hp->ht_next = NIL(HASH);
   return(hp);   
}



PUBLIC HASHPTR
Def_macro( name, value, flags )/*
=================================
   This routine is used to define a macro, and its value. A copy of
   the content of value is stored and not the pointer to the value.
   The flags indicates if it is a permanent macro or if its value
   can be redefined.  A flags of M_PRECIOUS means it is a precious
   macro and cannot be further redefined unless M_FORCE is used.
   If the flags flag contains the M_MULTI bit it means that the macro
   can be redefined multiple times and no warning of the redefinitions
   should be issued.
   Once a macro's VAR flags are set they are preserved through all future
   macro definitions.
   
   Macro definitions that have one of the variable bits set are treated
   specially.  In each case the hash table entry var field points at the
   global variable that can be set by assigning to the macro.
   
   bit valued global vars must be computed when the macro value is changed.
   char valued global vars must have the first char of ht_value copied to
   them.  string valued global vars have the same value as ht_value and should
   just have the new value of ht_value copied to them.  */

char    *name;			/* macro name to define	*/
char    *value;			/* macro value to set	*/
int     flags;			/* initial ht_flags	*/
{
   register HASHPTR   hp;
   register char      *p, *q;

   DB_ENTER( "Def_macro" );
   DB_PRINT( "mac", ("Defining macro %s = %s, %x", name, value, flags) );

   /* check to see if name is in the table, if so then just overwrite
      the previous definition.  Otherwise allocate a new node, and
      stuff it in the hash table, at the front of any linked list */

   if( Readenv ) flags |= M_LITERAL|M_EXPANDED;

   hp = Get_name( name, Macs, TRUE );

   if ((flags & M_PUSH) && hp->ht_name != NIL(char)) {
      HASHPTR thp=hp;
      TALLOC(hp,1,HASH);
      hp->ht_name  = DmStrDup(thp->ht_name);
      hp->ht_hash  = thp->ht_hash;
      Push_macro(hp);
      flags |= hp->ht_flag;
   }
   flags &= ~M_PUSH;

   if( (hp->ht_flag & M_PRECIOUS) && !(flags & M_FORCE) ) {
      if  (Verbose & V_WARNALL)
	     Warning( "Macro `%s' cannot be redefined", name );
      DB_RETURN( hp );
   }

   /* Make sure we don't export macros whose names contain legal macro
    * assignment operators, since we can't do proper quoting in the
    * environment. */
   if( *DmStrPbrk(name, "*+:=") != '\0' ) flags |= M_NOEXPORT;

   if( hp->ht_value != NIL(char) ) FREE( hp->ht_value );

   if( (hp->ht_flag & M_USED) && !((flags | hp->ht_flag) & M_MULTI) )
      Warning( "Macro `%s' redefined after use", name );

   /* If an empty string ("") is given set ht_value to NIL(char) */
   if( (value != NIL(char)) && (*value) ) {

      p = q = DmStrDup(value);
      if( !(flags & M_LITERAL) ) {

	 /* strip out any \<nl> combinations where \ is the current
	  * CONTINUATION char */
	 for(; (p=strchr(p,CONTINUATION_CHAR))!=NIL(char); )
	    if( p[1] == '\n' ) {
	       size_t len = strlen(p+2)+1;
	       memmove ( p, p+2, len );
	    }
	    else
	       p++;

	 p = DmStrSpn(q ," \t");	/* Strip white space before ... */
	 if( p != q ) {
	    size_t len = strlen(p)+1;
	    memmove( q, p, len );
	    p = q;
	 }

	 if( *p ) {			/* ... and after the value. */
	    for(q=p+strlen(p)-1; ((*q == ' ')||(*q == '\t')); q--);
	    *++q = '\0';
	 }
	 flags &= ~M_LITERAL;
      }
      /* else take string literally   */
      
      if( !*p )	{				/* check if result is ""   */
         FREE( p );
         p = NIL(char);
	 flags |= M_EXPANDED;
      }
      else if( *DmStrPbrk( p, "${}" ) == '\0' )
	 flags |= M_EXPANDED;

      hp->ht_value = p;
   }
   else {
      hp->ht_value = NIL(char);
      flags |= M_EXPANDED;
   }

   /* Assign the hash table flag less the M_MULTI flag, it is used only
    * to silence the warning.  But carry it over if it was previously
    * defined in ht_flag, as this is a permanent M_MULTI variable. Keep
    * the M_PRECIOUS flag and strip the M_INIT flag. */

   hp->ht_flag = ((flags & ~(M_MULTI|M_FORCE)) |
		  (hp->ht_flag & (M_VAR_MASK|M_MULTI|M_PRECIOUS))) & ~M_INIT;
   
   /* Check for macro variables and make the necessary adjustment in the
    * corresponding global variables */
    
   if( hp->ht_flag & M_VAR_MASK ) {
      if( !(flags & M_EXPANDED) )
	 Error( "Macro variable '%s' must be assigned with :=", name );
      else
         set_macro_value(hp);
   }

   DB_RETURN( hp );
}



PUBLIC CELLPTR
Def_cell( name )/*
==================
   Check if a cell for "name" already exists, if not create a new cell.
   The value of name is normalized before checking/creating the cell to
   avoid creating multiple cells for the same target file.
   The function returns a pointer to the cell. */
char    *name;
{
   register HASHPTR  hp;
   register CELLPTR  cp;
   register CELLPTR  lib;
   char		     *member;
   char		     *end;

   DB_ENTER( "Def_cell" );

   /* Check to see if the cell is a member of the form lib(member) or
    * lib((symbol)) and handle the cases appropriately.
    * What we do is we look at the target, if it is of the above two
    * forms we get the lib, and add the member/symbol to the list of
    * prerequisites for the library.  If this is a symbol name def'n
    * we additionally add the attribute A_SYMBOL, so that stat can
    * try to do the right thing.  */

   if( ((member = strchr(name, '('))     != NIL(char)) &&
       ((end    = strrchr(member,  ')')) != NIL(char)) &&
       (member > name) && (member[-1] != '$') &&
       (end > member+1)  && (end[1] == '\0') )
   {
      *member++ = *end = '\0';

      if( (*member == '(') && (member[strlen(member)-1] == ')') ) {
	 member[ strlen(member)-1 ] = '\0';
	 cp = Def_cell( member+1 );
	 cp->ce_attr |= A_SYMBOL;
      }
      else
	 cp = Def_cell( member );

      lib  = Def_cell( name );

      Add_prerequisite( lib, cp, FALSE, FALSE );
      lib->ce_attr |= A_LIBRARY | A_COMPOSITE;

      if( !Def_targets ) cp = lib;
   }
   else {
      /* Normalize the name. */
      DB_PRINT( "path", ("Normalizing [%s]", name) );

      /* The normalizing function returns a pointer to a static buffer. */
      name = normalize_path(name);

      hp = Get_name( name, Defs, TRUE );/* get the name from hash table */

      if( hp->CP_OWNR == NIL(CELL) )	/* was it previously defined    */
      {					/* NO, so define a new cell	*/
	 DB_PRINT( "cell", ("Defining cell [%s]", name) );

	 TALLOC( cp, 1, CELL );
	 hp->CP_OWNR = cp;
	 cp->ce_name = hp;
	 cp->ce_fname = hp->ht_name;
	 cp->ce_all.cl_prq = cp;
      }
      else 				/* YES, so return the old cell	*/
      {
	 DB_PRINT( "cell", ("Getting cell [%s]", hp->ht_name) );
	 cp = hp->CP_OWNR;
      }
   }

   DB_RETURN( cp );
}




PUBLIC LINKPTR
Add_prerequisite( cell, prq, head, force )/*
============================================
	Add a dependency node to the dag.  It adds it to the prerequisites,
	if any, of the cell and makes certain they are in linear order.
	If head == 1, then add to head of the prerequisite list, else
	add to tail. */
CELLPTR cell;
CELLPTR prq;
int     head;
int     force;
{
   register LINKPTR lp, tlp;

   DB_ENTER( "Add_prerequisite" );
   DB_PRINT( "cell", ("Defining prerequisite %s", prq->CE_NAME) );

   if( (prq->ce_flag & (F_MAGIC | F_PERCENT)) && !force )
      Fatal( "Special target [%s] cannot be a prerequisite",
             prq->CE_NAME );

   if( cell->ce_prq == NIL(LINK) ) {	/* it's the first one	*/
      TALLOC( lp, 1, LINK );
      lp->cl_prq   = prq;
      cell->ce_prq = lp;
   }
   else	{	/* search the list, checking for duplicates	*/
      for( lp = cell->ce_prq;
	   (lp->cl_next != NIL(LINK)) && (lp->cl_prq != prq);
	   lp = lp->cl_next );

      /* If the prq is not found and we are at the last prq in the list,
       * allocate a new prq and place it into the list, insert it at the
       * head if head == 1, else we add it to the end. */

      if( lp->cl_prq != prq ) {
	 TALLOC( tlp, 1, LINK );
	 tlp->cl_prq = prq;

	 if( head ) {
	    tlp->cl_next = cell->ce_prq;
	    cell->ce_prq = tlp;
	 }
	 else
	    lp->cl_next  = tlp;

	 lp = tlp;
      }
   }

   DB_RETURN( lp );
}



PUBLIC void
Clear_prerequisites( cell )/*
=============================
	Clear out the list of prerequisites, freeing all of the LINK nodes,
	and setting the list to NULL */
CELLPTR cell;
{
   LINKPTR lp, tlp;

   DB_ENTER( "Clear_prerequisites" );
   DB_PRINT( "cell", ("Nuking prerequisites") );

   if( cell == NIL(CELL) ) { DB_VOID_RETURN; }

   for( lp=cell->ce_prq; lp != NIL(LINK); lp=tlp ) {
      tlp=lp->cl_next;
      FREE( lp );
   }

   cell->ce_prq = NIL(LINK);

   DB_VOID_RETURN;
}


PUBLIC int
Test_circle( cp, fail )/*
=========================
	Actually run through the graph */
CELLPTR cp;
int     fail;
{
   register LINKPTR lp;
   int res = 0;

   DB_ENTER( "Test_circle" );
   DB_PRINT( "tc", ("checking [%s]", cp->CE_NAME) );

   if( cp->ce_flag & F_MARK ) {
      if( fail )
	 Fatal("Detected circular dependency in graph at [%s]", cp->CE_NAME);
      else
	 DB_RETURN( 1 );
   }

   cp->ce_flag |= F_MARK;
   for( lp = cp->ce_prq; !res && lp != NIL(LINK); lp = lp->cl_next )
      res = Test_circle( lp->cl_prq, fail );
   cp->ce_flag ^= F_MARK;

   DB_RETURN( res );
}



PUBLIC STRINGPTR
Def_recipe( rcp, sp, white_too, no_check )/*
=============================================
   Take the recipe (rcp) and add it to the list of recipes pointed to by
   sp (sp points to the last element).  If white_too == TRUE add the recipe
   even if it contains only white space or an empty string.
   Return a pointer to the new recipe (or sp if it was discarded).
   If no_check is true then don't look for -@ at the start of the recipe
   line. */
char      *rcp;
STRINGPTR sp;
int       white_too;
int       no_check;
{
   register STRINGPTR nsp;
   register char      *rp;

   DB_ENTER( "Def_recipe" );
   DB_PRINT( "rul", ("Defining recipe %s", rcp) );

   if( !white_too ) rcp = DmStrSpn( rcp, " \t" );
   if( (rcp == NIL(char)) || (*rcp == 0 && !white_too) )
      DB_RETURN( sp );	     /* return last recipe when new recipe not added */

   rp = no_check ? rcp : DmStrSpn( rcp, " \t@-+%" );

   TALLOC(nsp, 1, STRING);
   nsp->st_string = DmStrDup( rp );

   if( sp != NIL(STRING) ) sp->st_next = nsp;
   nsp->st_next = NIL(STRING);

   if( !no_check ) nsp->st_attr |= Rcp_attribute( rcp );

   DB_RETURN( nsp );
}


PUBLIC t_attr
Rcp_attribute( rp )/*
======================
   Look at the recipe and return the set of attributes that it defines. */
char *rp;
{
   t_attr flag = A_DEFAULT;
   int    done = FALSE;
   int atcount = 0;

   while( !done )
      switch( *rp++ )
      {
	 case '@' : ++atcount;        break;
	 case '-' : flag |= A_IGNORE; break;
	 case '+' : flag |= A_SHELL;  break;
	 case '%' :
#if defined(MSDOS) && defined(REAL_MSDOS)
	    /* Ignore % in the non-MSDOS case. */
	    flag |= A_SWAP;
#endif
	    break;

	 case ' ' :
	 case '\t': break;

	 default: done = TRUE; break;
      }

   if( !(Verbose & V_FORCEECHO) && atcount-- ) {
      flag |= A_SILENT;
      /* hide output if more than one @ are encountered. */
      if( atcount )
	 flag |= A_MUTE;
   }

   return(flag);
}
