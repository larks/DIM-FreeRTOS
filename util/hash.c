/*
 * Hash function
 */

#ifndef OSK
#	define register
#endif

/*#define HashVecSize		20 */	/* plenty for 31 character names */
#define HashVecSize		80	/* plenty for 132 character names */

typedef union {
    short int		intname[HashVecSize];	 /* name as vector of ints */
    char		charname[2*HashVecSize]; /* name as vector of chars */
} HashName;


int HashFunction(name, max)
char	*name;
int	max;
{
    HashName		locname;	/* aligned name */
    register int			namelen;	/* length of name */
    register int			namelim;	/* length limit (fullword size) */
    register int			namextra;	/* limit factor remainder */
    register int			code = 0;	/* hash code value */
    register int			ndx;		/* loop index */


    /*
     * Copy the name into the local aligned union.
     * Process the name as a vector of integers, with some remaining characters.
     * The string is copied into a local union in order to force correct
     * alignment for alignment-sensitive processors.
     */
    strcpy (locname.charname, name);
    namelen = strlen (locname.charname);
    namelim = namelen >> 1;		/* divide by 2 */
    namextra = namelen & 1;		/* remainder */

    /*
     * XOR each integer part of the name together, followed by the trailing
     * 0/1 character
     */
    for( ndx=0; ndx < namelim; ndx++ )
        code = code ^ ((locname.intname[ndx]) << ndx);
    if( namextra > 0 )
        code = code ^ ((locname.intname[ndx]) & 0x00FF);

    return (code & 0x7FFF) % max;
}

