/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
# include	"trek.h"

void
sector(
        int	*x, 
		int *y)
{
	register int		i, j;

	do {
		i = ranf(NSECTS);
		j = ranf(NSECTS);
	} while (Sect[i][j] != EMPTY);
	*x = i;
	*y = j;
}

/*
**  Paramize Quadrant Upon Entering
**
**	A quadrant is initialized from the information held in the
**	Quad matrix.  Basically, everything is just initialized
**	randomly, except for the starship, which goes into a fixed
**	sector.
**
**	If there are Klingons in the quadrant, the captain is informed
**	that the condition is RED, and he is given a chance to put
**	his shields up if the computer is working.
**
**	The flag `f' is set to disable the check for condition red.
**	This mode is used in situations where you know you are going
**	to be docked, i.e., abandon() and help().
*/
void
initquad(
        int	f)
{
	register int		i, j;
	int			rx, ry;
	int			nbases, nstars;
	register struct quad	*q;
	int			nholes;

	q = &Quad[Ship.quadx][Ship.quady];

	/* ignored supernova'ed quadrants (this is checked again later anyway */
	if (q->stars < 0)
		return;
	Etc.nkling = q->klings;
	nbases = q->bases;
	nstars = q->stars;
	nholes = q->holes;

	/* have we blundered into a battle zone w/ shields down? */
	if (Etc.nkling > 0 && !f)
	{
		printf("Condition RED\n");
		Ship.cond = RED;
		if (!damaged(COMPUTER))
			shield(1);
	}

	/* clear out the quadrant */
	for (i = 0; i < NSECTS; i++)
		for (j = 0; j < NSECTS; j++)
			Sect[i][j] = EMPTY;

	/* initialize Enterprise */
	Sect[Ship.sectx][Ship.secty] = Ship.ship;

	/* initialize Klingons */
	for (i = 0; i < Etc.nkling; i++)
	{
		sector(&rx, &ry);
		Sect[rx][ry] = KLINGON;
		Etc.klingon[i].x = rx;
		Etc.klingon[i].y = ry;
		Etc.klingon[i].power = Param.klingpwr;
		Etc.klingon[i].srndreq = 0;
	}
	compkldist(1);

	/* initialize star base */
	if (nbases > 0)
	{
		sector(&rx, &ry);
		Sect[rx][ry] = BASE;
		Etc.starbase.x = rx;
		Etc.starbase.y = ry;
	}

	/* initialize inhabited starsystem */
	if (q->qsystemname != 0)
	{
		sector(&rx, &ry);
		Sect[rx][ry] = INHABIT;
		nstars -= 1;
	}

	/* initialize black holes */
	for (i = 0; i < nholes; i++)
	{
		sector(&rx, &ry);
		Sect[rx][ry] = HOLE;
	}

	/* initialize stars */
	for (i = 0; i < nstars; i++)
	{
		sector(&rx, &ry);
		Sect[rx][ry] = STAR;
	}
	Move.newquad = 1;
}
