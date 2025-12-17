#ifndef ISO7816_4_H
#define ISO7816_4_H

#include <stdint.h>

// ISO 7816-4 Status Words (SW)
#define SW_OK 0x9000
#define SW_WRONG_LENGTH 0x6700
#define SW_CONDITIONS_NOT_SATISFIED 0x6985
#define SW_WRONG_DATA 0x6A80
#define SW_FUNC_NOT_SUPPORTED 0x6A81
#define SW_FILE_NOT_FOUND 0x6A82
#define SW_INCORRECT_P1P2 0x6A86
#define SW_INS_NOT_SUPPORTED 0x6D00
#define SW_CLA_NOT_SUPPORTED 0x6E00
#define SW_UNKNOWN 0x6F00

// APDU positions
#define APDU_CLA_POS 0
#define APDU_INS_POS 1
#define APDU_P1_POS 2
#define APDU_P2_POS 3
#define APDU_LC_POS 4
#define APDU_DATA_POS 5

/**
 * @brief Sets a Status Word in the response buffer.
 *
 * @param buffer Output buffer.
 * @param len Pointer to the current length (will be set to 2).
 * @param sw The status word (e.g., SW_OK).
 */
void iso7816_set_sw(uint8_t *buffer, uint16_t *len, uint16_t sw);

/**
 * @brief Appends data to a response buffer and sets the Status Word.
 *
 * @param buffer Output buffer.
 * @param data_len Length of data to return.
 * @param total_len Pointer to the total length (will be data_len + 2).
 * @param sw The status word.
 */
void iso7816_finalize_response(uint8_t *buffer, uint16_t data_len,
                               uint16_t *total_len, uint16_t sw);

#endif // ISO7816_4_H
