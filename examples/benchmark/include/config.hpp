#ifndef CONFIG_HPP
#define CONFIG_HPP

/* Common defines for all collectives */
#define BUF_COUNT         (16)
#define ELEM_COUNT        (128)
#define ITERS             (16)
#define SINGLE_ELEM_COUNT (BUF_COUNT * ELEM_COUNT)
#define ALIGNMENT         (2 * 1024 * 1024)
#define DTYPE             float
#define MATCH_ID_SIZE     (256)

#endif /* CONFIG_HPP */
