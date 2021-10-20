#ifndef _UTILS_H_
#define _UTILS_H_

#define SET_BITS(reg, bit_mask)	\
	do{ \
		reg |= bit_mask; \
	}while(0)

#define CLEAR_BITS(reg, bit_mask)	\
	do{ \
		reg &= ~bit_mask; \
	}while(0)

#define MODIFY_REG(reg, bit_mask, new_value)	\
	do {	\
		reg = (reg & ~bit_mask) | (new_value & bit_mask);	\
	} while (0)
	
#define ARE_BITS_SET(reg, bit_mask)		((reg & bit_mask) != 0)
			
#endif // _UTILS_H_