/*
 * Simple Bitmap table for 8x5 characters
 * --------------------------------------
 *
 * Each chunk of 5 bytes represents one column of the
 * character. Each bit within the byte represents if
 * that pixel should be filled or not. Bit 7 is the
 * bottom line of the character.
 *
 * To access the data for a character, you can use the
 * code:
 *    
 *    BF_fontMap[requiredCharacter - ' '][line]
 *
 * For example if you wanted to print the character 'A'
 * you would plot each of the 5 columns:
 *                                             76543210
 *    BF_fontMap['A'-' '][0]; //First col       #######
 *    BF_fontMap['A'-' '][1]; //Second col         #  #
 *    BF_fontMap['A'-' '][2]; //Third col          #  #
 *    BF_fontMap['A'-' '][3]; //Forth col          #  #
 *    BF_fontMap['A'-' '][4]; //Fifth col       #######
 *
 * You might want to create a function to print one of
 * the character columns, which is called 5 times by
 * another function to print the whole character.
 *
 */

#ifndef BASICFONT_H_
#define BASICFONT_H_

#define numberOfCustomCharacters 2 //How many non-ASCII characters. Add to end of table

extern signed char BF_fontMap[96 + numberOfCustomCharacters][5];

#endif //BASICFONT_H_
