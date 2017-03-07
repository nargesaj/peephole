/*
 * JOOS is Copyright (C) 1997 Laurie Hendren & Michael I. Schwartzbach
 *
 * Reproduction of all or part of this software is permitted for
 * educational or research use on condition that this copyright notice is
 * included in any copy. This software comes with no warranty of any
 * kind. In no event will the authors be liable for any damages resulting from
 * use of this software.
 *
 * email: hendren@cs.mcgill.ca, mis@brics.dk
 */

/* iload x        iload x        iload x
 * ldc 0          ldc 1          ldc 2
 * imul           imul           imul
 * ------>        ------>        ------>
 * ldc 0          iload x        iload x
 *                               dup
 *                               iadd
 */

int simplify_multiplication_right(CODE **c)
{ int x,k;
  if (is_iload(*c,&x) && 
      is_ldc_int(next(*c),&k) && 
      is_imul(next(next(*c)))) {
     if (k==0) return replace(c,3,makeCODEldc_int(0,NULL));
     else if (k==1) return replace(c,3,makeCODEiload(x,NULL));
     else if (k==2) return replace(c,3,makeCODEiload(x,
                                       makeCODEdup(
                                       makeCODEiadd(NULL))));
     return 0;
  }
  return 0;
}

/* dup
 * astore x
 * pop
 * -------->
 * astore x
 */
int simplify_astore(CODE **c)
{ int x;
  if (is_dup(*c) &&
      is_astore(next(*c),&x) &&
      is_pop(next(next(*c)))) {
     return replace(c,3,makeCODEastore(x,NULL));
  }
  return 0;
}

/* iload x
 * ldc k   (0<=k<=127)
 * iadd
 * istore x
 * --------->
 * iinc x k
 */ 
int positive_increment(CODE **c)
{ int x,y,k;
  if (is_iload(*c,&x) &&
      is_ldc_int(next(*c),&k) &&
      is_iadd(next(next(*c))) &&
      is_istore(next(next(next(*c))),&y) &&
      x==y && 0<=k && k<=127) {
     return replace(c,4,makeCODEiinc(x,k,NULL));
  }
  return 0;
}

/* goto L1
 * ...
 * L1:
 * goto L2
 * ...
 * L2:
 * --------->
 * goto L2
 * ...
 * L1:    (reference count reduced by 1)
 * goto L2
 * ...
 * L2:    (reference count increased by 1)  
 */
int simplify_goto_goto(CODE **c)
{ int l1,l2;
  if (is_goto(*c,&l1) && is_goto(next(destination(l1)),&l2) && l1>l2) {
     droplabel(l1);
     copylabel(l2);
     return replace(c,1,makeCODEgoto(l2,NULL));
  }
  return 0;
}
/*---------------------------------------------------------------------*/
/* dup
 * istore x
 * pop
 * -------->
 * istore x
 */
int simplify_istore(CODE **c)
{ int x;
  if (is_dup(*c) &&
      is_istore(next(*c),&x) &&
      is_pop(next(next(*c)))) {
     return replace(c,3,makeCODEistore(x,NULL));
  }
  return 0;
}

/*
 * iload x			
 * iload x
 * imul
 * --------->
 * iload x
 * dup
 * imul
 */
int replace_slow_multiplication(CODE **c)
{
	int x, y;
	if (is_iload(*c, &x) &&
		is_iload(next(*c), &y) &&
		is_imul(next(next(*c))) &&
		x == y)
	{
		return replace(c, 3, makeCODEiload(x, 
							 makeCODEdup(
							 makeCODEimul(NULL))));
	}
	return 0;
}

/*
 * ldc k
 * ldc k
 * --------->
 * ldc k
 * dup
 */
int push_dup_constant_to_stack(CODE **c)
{
	int k, l;
	if (is_ldc_int(*c, &k) &&
		is_ldc_int(next(*c), &l) && 
		k > 5 && k == l)
	{
		return replace(c, 2, makeCODEldc_int(k,
							 makeCODEdup(NULL)));
	}
	return 0;
}

/*
 * ldc 0
 * iadd
 * --------->
 * 
 */
int remove_add_by_zero(CODE **c)
{
	int k;
	if (is_ldc_int(*c, &k) && (k==0) &&
		is_iadd(next(*c)))
	{
		return replace(c, 2, NULL);
	}
	return 0;
}

/*
 * if_icmplt L1
 * ldc 0
 * goto L2
 * L1:
 * ldc 1
 * L2:
 * ifeq L3
 * ----------------->
 * if_icmpge L3
 */
int simplify_if_icmplt(CODE **c)
{
	int l1, l2, l3;
	int k, m;
	if (is_if_icmplt(*c, &l1) &&
	    (is_ldc_int(next(destination(l1)), &m) && (m==1)) && 
		(is_ldc_int(next(*c), &k) && (k==0)) &&
		is_goto(next(next(*c)), &l2) &&
		is_ifeq(next(destination(l2)), &l3) && l2>l3)
	{
		droplabel(l1);
		droplabel(l2);
		copylabel(l3);
		return replace(c, 7, makeCODEif_icmpge(l3, NULL));
	}
	return 0;
}

/*
 * if_icmple L1
 * ldc 0
 * goto L2
 * L1:
 * ldc 1
 * L2:
 * ifeq L3
 * ----------------->
 * if_icmpgt L3
 */
int simplify_if_icmple(CODE **c)
{
	int l1, l2, l3;
	int k, m;
	if (is_if_icmple(*c, &l1) &&
	    (is_ldc_int(next(destination(l1)), &m) && (m==1)) && 
		(is_ldc_int(next(*c), &k) && (k==0)) &&
		is_goto(next(next(*c)), &l2) &&
		is_ifeq(next(destination(l2)), &l3) && l2>l3)
	{
		droplabel(l1);
		droplabel(l2);
		copylabel(l3);
		return replace(c, 7, makeCODEif_icmpgt(l3, NULL));
	}
	return 0;
}

/*
 * if_icmpgt L1
 * ldc 0
 * goto L2
 * L1:
 * ldc 1
 * L2:
 * ifeq L3
 * ----------------->
 * if_icmple L3
 */
int simplify_if_icmpgt(CODE **c)
{
	int l1, l2, l3;
	int k, m;
	if (is_if_icmpgt(*c, &l1) &&
	    (is_ldc_int(next(destination(l1)), &m) && (m==1)) && 
		(is_ldc_int(next(*c), &k) && (k==0)) &&
		is_goto(next(next(*c)), &l2) &&
		is_ifeq(next(destination(l2)), &l3) && l2>l3)
	{
		droplabel(l1);
		droplabel(l2);
		copylabel(l3);
		return replace(c, 7, makeCODEif_icmple(l3, NULL));
	}
	return 0;
}

/*
 * if_icmpge L1
 * ldc 0
 * goto L2
 * L1:
 * ldc 1
 * L2:
 * ifeq L3
 * ----------------->
 * if_icmplt L3
 */
int simplify_if_icmpge(CODE **c)
{
	int l1, l2, l3;
	int k, m;
	if (is_if_icmpge(*c, &l1) &&
	    (is_ldc_int(next(destination(l1)), &m) && (m==1)) && 
		(is_ldc_int(next(*c), &k) && (k==0)) &&
		is_goto(next(next(*c)), &l2) &&
		is_ifeq(next(destination(l2)), &l3) && l2>l3)
	{
		droplabel(l1);
		droplabel(l2);
		copylabel(l3);
		return replace(c, 7, makeCODEif_icmplt(l3, NULL));
	}
	return 0;
}

/*
 * if_icmpeq L1
 * ldc 0
 * goto L2
 * L1:
 * ldc 1
 * L2:
 * ifeq L3
 * ----------------->
 * if_icmpne L3
 */
int simplify_if_icmpeq(CODE **c)
{
	int l1, l2, l3;
	int k, m;
	if (is_if_icmpeq(*c, &l1) &&
	    (is_ldc_int(next(destination(l1)), &m) && (m==1)) && 
		(is_ldc_int(next(*c), &k) && (k==0)) &&
		is_goto(next(next(*c)), &l2) &&
		is_ifeq(next(destination(l2)), &l3) && l2>l3)
	{
		droplabel(l1);
		droplabel(l2);
		copylabel(l3);
		return replace(c, 7, makeCODEif_icmpne(l3, NULL));
	}
	return 0;
}

/*
 * if_icmpne L1
 * ldc 0
 * goto L2
 * L1:
 * ldc 1
 * L2:
 * ifeq L3
 * ----------------->
 * if_icmpeq L3
 */
int simplify_if_icmpne(CODE **c)
{
	int l1, l2, l3;
	int k, m;
	if (is_if_icmpne(*c, &l1) &&
	    (is_ldc_int(next(destination(l1)), &m) && (m==1)) && 
		(is_ldc_int(next(*c), &k) && (k==0)) &&
		is_goto(next(next(*c)), &l2) &&
		is_ifeq(next(destination(l2)), &l3) && l2>l3)
	{
		droplabel(l1);
		droplabel(l2);
		copylabel(l3);
		return replace(c, 7, makeCODEif_icmpeq(l3, NULL));
	}
	return 0;
}


/*
 * iload x
 * istore x
 * --------->
 * 
 */

int remove_iload_x_istore_x(CODE **c)
{
	int x, y;
	if (is_iload(*c, &x) &&
		is_istore(next(*c), &y) &&
		x == y)
	{
		return replace(c, 2, NULL);
	}
	
	return 0;
}

/*
 * aload x
 * astore x
 * --------->
 * 
 */

int remove_aload_x_astore_x(CODE **c)
{
	int x, y;
	if (is_aload(*c, &x) &&
		is_astore(next(*c), &y) &&
		x == y)
	{
		return replace(c, 2, NULL);
	}
	
	return 0;
}

/*
 * ldc k
 * ldc l
 * iadd 
 * --------->
 * ldc m
 */
int Constant_folding(CODE **c)
{
	int k, l;
	if (is_ldc_int(*c, &k) &&
		is_ldc_int(next(*c), &l) &&
		is_iadd(next(next(*c))) && (k+l < 256))
	{
		return replace(c, 3, makeCODEldc_int(k+l, NULL));
	}
	return 0;
}

/*
 * dup
 * ifnull L1
 * goto L2
 * L1:
 * pop
 * ldc "null"
 * L2:
 * ----------->
 * dup
 * ifnonnull L2
 * pop
 * ldc "null"
 * L2:
 */
int simplify_ifnull_comparison(CODE **c)
{
	int l1, l2;
	char* ch;
	if (is_dup(*c) &&
		is_ifnull(next(*c), &l1) &&
		is_pop(next(destination(l1))) &&
		(is_ldc_string(next(next(destination(l1))), &ch) && strcmp(ch,"null")==0) &&
		is_goto(next(next(*c)), &l2))
	{
		droplabel(l1);
		copylabel(l2);
		
		return replace(c, 4, makeCODEdup(
							 makeCODEifnonnull(l2, NULL)));/*
							 makeCODEpop(
							 makeCODEldc_string("null", NULL)))));*/
	}
	return 0;
}

/*
 * if_icmge L1
 * ldc 0
 * goto L2
 * L1:
 * ldc 1
 * L2:
 * dup
 * ifne L3
 * pop
 * ------------->
 * if_icmplt L2
 * ldc 1
 * goto L3
 * L2:
 */

int simplify_if_icmpge_with_dup(CODE **c)
{
	int l1, l2, l3;
	int k, m;
	if (is_if_icmpge(*c, &l1) &&
		(is_ldc_int(next(destination(l1)), &m) && (m==1)) &&
		(is_ldc_int(next(*c), &k) && (k==0)) &&
		is_goto(next(next(*c)), &l2) &&
		is_dup(next(destination(l2))) &&
		(is_ifne(next(next(destination(l2))), &l3) && l2>l3) &&
		is_pop(next(next(next(destination(l2))))))
	{
		droplabel(l1);
		copylabel(l2);
		copylabel(l3);
		
		return replace(c, 9, makeCODEif_icmplt(l2,
							 makeCODEldc_int(1,
							 makeCODEgoto(l3,
							 makeCODElabel(l2, NULL)))));
	}
	return 0;
}

/*
 * if_icmge L1
 * ldc 0
 * goto L2
 * L1:
 * ldc 1
 * L2:
 * L4:
 * dup
 * ifne L3
 * pop
 * ------------->
 * if_icmplt L2
 * ldc 1
 * L4:
 * goto L3
 * L2:
 */

int simplify_if_icmpge_with_dup_2label(CODE **c)
{
	int l1, l2, l3, l4;
	int k, m;
	if (is_if_icmpge(*c, &l1) &&
		(is_ldc_int(next(destination(l1)), &m) && (m==1)) &&
		(is_ldc_int(next(*c), &k) && (k==0)) &&
		is_goto(next(next(*c)), &l2) &&
		is_label(next(destination(l2)), &l4) && 
		is_dup(next(next(destination(l2)))) &&
		(is_ifne(next(next(next(destination(l2)))), &l3)) && 
		is_pop(next(next(next(next(destination(l2)))))))
	{
		droplabel(l1);
		copylabel(l2);
		copylabel(l3);
		copylabel(l4);
		
		return replace(c, 10, makeCODEif_icmplt(l2,
							  makeCODEldc_int(1,
							  makeCODElabel(l4,
							  makeCODEgoto(l3,
							  makeCODElabel(l2, NULL))))));
							/*makeCODElabel(l2, NULL)))));*/
	}
	return 0;
}

/*
 * ifeq L1
 * ldc 0
 * goto L2
 * L1:
 * ldc 1
 * L2:
 * ifeq L3
 * --------->
 * ifne L3
 */
int simplify_ifeq(CODE **c)
{
	int l1, l2, l3;
	int k, m;
	
	if (is_ifeq(*c, &l1) &&
		(is_ldc_int(next(destination(l1)), &m) && (m==1)) && 
		(is_ldc_int(next(*c), &k) && (k==0)) &&
		is_goto(next(next(*c)), &l2) &&
		is_ifeq(next(destination(l2)), &l3) && l2>l3)
	{
		droplabel(l1);
		droplabel(l2);
		copylabel(l3);
		return replace(c, 7, makeCODEifne(l3, NULL));
	}
	return 0;
}

/*
 * L1:
 * --------->
 */
int remove_deadlabel(CODE **c)
{
	int l;
	
	if (is_label(*c, &l) && deadlabel(l))
	{
		droplabel(l);
		
		return replace(c, 1, NULL);
	}
	return 0;
}

/*
 * nop
 * --------->
 */
int remove_nop(CODE **c)
{
	if (is_nop(*c))
	{
		return replace(c, 1, NULL);
	}
	return 0;
}

/*
 * iload x
 * iload x
 * ----------->
 * iload x
 * dup
 */
int iload_folding(CODE **c)
{
	int x, y;
	
	if (is_iload(*c, &x) &&
		is_iload(next(*c), &y) && x > 5 && x==y)
	{
		return replace(c, 2, makeCODEiload(x,
							 makeCODEdup(NULL)));
	}
	return 0;
}

/*
 * aload x
 * aload x
 * ----------->
 * aload x
 * dup
 */
int aload_folding(CODE **c)
{
	int x, y;
	
	if (is_aload(*c, &x) &&
		is_aload(next(*c), &y) && x==y)
	{
		return replace(c, 2, makeCODEaload(x,
							 makeCODEdup(NULL)));
	}
	return 0;
}

/*
 * ldc 1
 * idiv 
 * --------->
 */
int remove_divide_by_one(CODE **c)
{
	int k;
	
	if (is_ldc_int(*c, &k) && (k==1) &&
		is_idiv(next(*c)))
	{
		return replace(c, 2, NULL);
	}
	return 0;
}

/*
 * new x
 * dup
 * invokenonvirtual y
 * dup
 * aload k
 * swap
 * putfield z
 * pop
 * -------------->
 * aload k
 * new x
 * dup
 * invokenonvirtual y
 * putfield z
 */
int simplify_invokenonvirtual(CODE **c)
{
	char* ch_new;
	char* ch_invoke;
	char* ch_put;
	int k;
	
	if (is_new(*c, &ch_new) &&
		is_dup(next(*c)) &&
		is_invokenonvirtual(next(next(*c)), &ch_invoke) &&
		is_dup(next(next(next(*c)))) &&
		is_aload(next(next(next(next(*c)))), &k) &&
		is_swap(next(next(next(next(next(*c)))))) &&
		is_putfield(next(next(next(next(next(next(*c)))))), &ch_put) &&
		is_pop(next(next(next(next(next(next(next(*c)))))))))
	{
		return replace(c, 8, makeCODEaload(k,
							 makeCODEnew(ch_new,
							 makeCODEdup(
							 makeCODEinvokenonvirtual(ch_invoke,
							 makeCODEputfield(ch_put, NULL))))));		
	}
	return 0;
}

/*
 * aload x
 * dup
 * aload y
 * swap
 * putfield
 * pop
 * ---------->
 * aload y
 * aload x
 * putfield
 */
int simplify_swap_aload_putfield(CODE **c)
{
	int x, y;
	char *ch;
	
	if (is_aload(*c, &x) &&
		is_dup(next(*c)) &&
		is_aload(next(next(*c)), &y) &&
		is_swap(next(next(next(*c)))) &&
		is_putfield(next(next(next(next(*c)))), &ch) &&
		is_pop(next(next(next(next(next(*c))))))) 
	{
		return replace(c, 6, makeCODEaload(y,
							 makeCODEaload(x,
							 makeCODEputfield(ch, NULL))));
	}
	return 0;
}

/*
 * iadd
 * dup
 * aload x
 * swap
 * putfield ch
 * pop
 * ---------->
 * iadd
 * aload x
 * swap
 * putfield
 */
int simplify_swap_iadd_putfield(CODE **c)
{
	int x;
	char *ch;
	
	if (is_iadd(*c) &&
		is_dup(next(*c)) &&
		is_aload(next(next(*c)), &x) &&
		is_swap(next(next(next(*c)))) &&
		is_putfield(next(next(next(next(*c)))), &ch) &&
		is_pop(next(next(next(next(next(*c))))))) 
	{
		return replace(c, 6, makeCODEiadd(
							 makeCODEaload(x,
							 makeCODEswap(
							 makeCODEputfield(ch, NULL)))));
	}
	return 0;
}

/*
 * isub
 * dup
 * aload x
 * swap
 * putfield ch
 * pop
 * ---------->
 * isub
 * aload x
 * swap
 * putfield
 */
int simplify_swap_isub_putfield(CODE **c)
{
	int x;
	char *ch;
	
	if (is_isub(*c) &&
		is_dup(next(*c)) &&
		is_aload(next(next(*c)), &x) &&
		is_swap(next(next(next(*c)))) &&
		is_putfield(next(next(next(next(*c)))), &ch) &&
		is_pop(next(next(next(next(next(*c))))))) 
	{
		return replace(c, 6, makeCODEisub(
							 makeCODEaload(x,
							 makeCODEswap(
							 makeCODEputfield(ch, NULL)))));
	}
	return 0;
}

/*
 * imul
 * dup
 * aload x
 * swap
 * putfield ch
 * pop
 * ---------->
 * imul
 * aload x
 * swap
 * putfield
 */
int simplify_swap_imul_putfield(CODE **c)
{
	int x;
	char *ch;
	
	if (is_imul(*c) &&
		is_dup(next(*c)) &&
		is_aload(next(next(*c)), &x) &&
		is_swap(next(next(next(*c)))) &&
		is_putfield(next(next(next(next(*c)))), &ch) &&
		is_pop(next(next(next(next(next(*c))))))) 
	{
		return replace(c, 6, makeCODEimul(
							 makeCODEaload(x,
							 makeCODEswap(
							 makeCODEputfield(ch, NULL)))));
	}
	return 0;
}

/*
 * idiv
 * dup
 * aload x
 * swap
 * putfield ch
 * pop
 * ---------->
 * idiv
 * aload x
 * swap
 * putfield
 */
int simplify_swap_idiv_putfield(CODE **c)
{
	int x;
	char *ch;
	
	if (is_idiv(*c) &&
		is_dup(next(*c)) &&
		is_aload(next(next(*c)), &x) &&
		is_swap(next(next(next(*c)))) &&
		is_putfield(next(next(next(next(*c)))), &ch) &&
		is_pop(next(next(next(next(next(*c))))))) 
	{
		return replace(c, 6, makeCODEidiv(
							 makeCODEaload(x,
							 makeCODEswap(
							 makeCODEputfield(ch, NULL)))));
	}
	return 0;
}

/*
 * iload x
 * dup
 * aload y
 * swap
 * putfield
 * pop
 * ---------->
 * aload y
 * iload x
 * putfield
 */
int simplify_swap_iload_putfield(CODE **c)
{
	int x, y;
	char *ch;
	
	if (is_iload(*c, &x) &&
		is_dup(next(*c)) &&
		is_aload(next(next(*c)), &y) &&
		is_swap(next(next(next(*c)))) &&
		is_putfield(next(next(next(next(*c)))), &ch) &&
		is_pop(next(next(next(next(next(*c))))))) 
	{
		return replace(c, 6, makeCODEaload(y,
							 makeCODEiload(x,
							 makeCODEputfield(ch, NULL))));
	}
	return 0;
}

/*
 * ldc x
 * dup
 * aload y
 * swap
 * putfield
 * pop
 * ---------->
 * aload y
 * ldc x
 * putfield
 */
int simplify_swap_ldc_putfield(CODE **c)
{
	int x, y;
	char *ch;
	
	if (is_ldc_int(*c, &x) &&
		is_dup(next(*c)) &&
		is_aload(next(next(*c)), &y) &&
		is_swap(next(next(next(*c)))) &&
		is_putfield(next(next(next(next(*c)))), &ch) &&
		is_pop(next(next(next(next(next(*c))))))) 
	{
		return replace(c, 6, makeCODEaload(y,
							 makeCODEldc_int(x,
							 makeCODEputfield(ch, NULL))));
	}
	return 0;
}

/*
 * ldc "x"
 * dup
 * aload y
 * swap
 * putfield
 * pop
 * ---------->
 * aload y
 * ldc "x"
 * putfield
 */
int simplify_swap_ldc_string_putfield(CODE **c)
{
	int y;
	char *ch;
	char *x;
	
	if (is_ldc_string(*c, &x) &&
		is_dup(next(*c)) &&
		is_aload(next(next(*c)), &y) &&
		is_swap(next(next(next(*c)))) &&
		is_putfield(next(next(next(next(*c)))), &ch) &&
		is_pop(next(next(next(next(next(*c))))))) 
	{
		return replace(c, 6, makeCODEaload(y,
							 makeCODEldc_string(x,
							 makeCODEputfield(ch, NULL))));
	}
	return 0;
}

#define OPTS 35

OPTI optimization[OPTS] = {/* Laurie's patterns */
			   simplify_multiplication_right,
                           simplify_astore,
                           positive_increment,
                           simplify_goto_goto,
                           /* My patterns */
                           simplify_istore,
                           replace_slow_multiplication,
                           push_dup_constant_to_stack,
                           remove_add_by_zero,
                           simplify_if_icmplt, 
                           simplify_if_icmpgt, 
                           simplify_if_icmpeq, 
                           simplify_if_icmpne, 
                           simplify_if_icmpge, 
                           simplify_if_icmple, 
                           simplify_ifnull_comparison,
                           simplify_if_icmpge_with_dup,
                           simplify_ifeq,
                           remove_deadlabel,
                           iload_folding,
                           aload_folding,
                           remove_nop,
                           remove_divide_by_one,
                           simplify_if_icmpge_with_dup_2label,
                           simplify_swap_aload_putfield,
                           simplify_swap_iload_putfield,
                           simplify_swap_ldc_putfield,
                           simplify_swap_ldc_string_putfield,
                           simplify_invokenonvirtual,
                           Constant_folding,
                           remove_iload_x_istore_x,
                           remove_aload_x_astore_x,
                           simplify_swap_iadd_putfield,
                           simplify_swap_isub_putfield,
                           simplify_swap_imul_putfield,
                           simplify_swap_idiv_putfield};
