#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>

#include "heatshrink_encoder.h"
#include "heatshrink_decoder.h"

#define DEF_WINDOW_SZ2 11
#define DEF_LOOKAHEAD_SZ2 4
#define DEF_DECODER_INPUT_BUFFER_SIZE 256
#define DEF_BUFFER_SIZE (64 * 1024)

#if 0
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(...) /* NO-OP */
#endif

#if _WIN32
#include <errno.h>
#define HEATSHRINK_ERR(retval, ...) do { \
fprintf(stderr, __VA_ARGS__); \
fprintf(stderr, "Undefined error: %d\n", errno); \
exit(retval); \
} while(0)
#else
#include <err.h>
#define HEATSHRINK_ERR(...) err(__VA_ARGS__)
#endif

/*
 * We have to open binary files with the O_BINARY flag on Windows. Most other
 * platforms don't differentiate between binary and non-binary files.
 */
#ifndef O_BINARY
#define O_BINARY 0
#endif

static const int version_major = HEATSHRINK_VERSION_MAJOR;
static const int version_minor = HEATSHRINK_VERSION_MINOR;
static const int version_patch = HEATSHRINK_VERSION_PATCH;
static const char author[] = HEATSHRINK_AUTHOR;
static const char url[] = HEATSHRINK_URL;

static void usage(void) {
    fprintf(stderr, "heatshrink version %u.%u.%u by %s\n",
        version_major, version_minor, version_patch, author);
    fprintf(stderr, "Home page: %s\n\n", url);
    fprintf(stderr,
        "Usage:\n"
        "  heatshrink [-h] [-e|-d] [-v] [-w SIZE] [-l BITS] [IN_FILE] [OUT_FILE]\n"
        "\n"
        "heatshrink compresses or decompresses byte streams using LZSS, and is\n"
        "designed especially for embedded, low-memory, and/or hard real-time\n"
        "systems.\n"
        "\n"
        " -h        print help\n"
        " -e        encode (compress, default)\n"
        " -d        decode (decompress)\n"
        " -v        verbose (print input & output sizes, compression ratio, etc.)\n"
        "\n"
        " -w SIZE   Base-2 log of LZSS sliding window size\n"
        "\n"
        "    A larger value allows searches a larger history of the data for repeated\n"
        "    patterns, potentially compressing more effectively, but will use\n"
        "    more memory and processing time.\n"
        "    Recommended default: -w 8 (embedded systems), -w 10 (elsewhere)\n"
        "  \n"
        " -l BITS   Number of bits used for back-reference lengths\n"
        "\n"
        "    A larger value allows longer substitutions, but since all\n"
        "    back-references must use -w + -l bits, larger -w or -l can be\n"
        "    counterproductive if most patterns are small and/or local.\n"
        "    Recommended default: -l 4\n"
        "\n"
        " If IN_FILE or OUT_FILE are unspecified, they will default to\n"
        " \"-\" for standard input and standard output, respectively.\n");
    exit(1);
}

typedef enum { IO_READ, IO_WRITE, } IO_mode;
typedef enum { OP_ENC, OP_DEC, } Operation;

typedef struct {
    int fd;                     /* file descriptor */
    IO_mode mode;
    size_t fill;                /* fill index */
    size_t read;                /* read index */
    size_t size;
    size_t total;
    uint8_t buf[];
} io_handle;

typedef struct {
    uint8_t window_sz2;
    uint8_t lookahead_sz2;
    size_t decoder_input_buffer_size;
    size_t buffer_size;
    uint8_t verbose;
    Operation cmd;
    char *in_fname;
    char *out_fname;
    io_handle *in;
    io_handle *out;
} config;

static void die(char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

static void report(config *cfg);

/* Open an IO handle. Returns NULL on error. */
static io_handle *handle_open(char *fname, IO_mode m, size_t buf_sz) {
    io_handle *io = NULL;
    io = malloc(sizeof(*io) + buf_sz);
    if (io == NULL) { return NULL; }
    memset(io, 0, sizeof(*io) + buf_sz);
    io->fd = -1;
    io->size = buf_sz;
    io->mode = m;

    if (m == IO_READ) {
        if (0 == strcmp("-", fname)) {
            io->fd = STDIN_FILENO;
        } else {
            io->fd = open(fname, O_RDONLY | O_BINARY);
        }
    } else if (m == IO_WRITE) {
        if (0 == strcmp("-", fname)) {
            io->fd = STDOUT_FILENO;
        } else {
            io->fd = open(fname, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC /*| O_EXCL*/, 0644);
        }
    }

    if (io->fd == -1) {         /* failed to open */
        free(io);
        HEATSHRINK_ERR(1, "open");
        return NULL;
    }

    return io;
}

/* Read SIZE bytes from an IO handle and return a pointer to the content.
 * BUF contains at least size_t bytes. Returns 0 on EOF, -1 on error. */
static ssize_t handle_read(io_handle *io, size_t size, uint8_t **buf) {
    LOG("@ read %zd\n", size);
    if (buf == NULL) { return -1; }
    if (size > io->size) {
        fprintf(stderr, "size %zd, io->size %zd\n", size, io->size);
        return -1;
    }
    if (io->mode != IO_READ) { return -1; }

    size_t rem = io->fill - io->read;
    if (rem >= size) {
        *buf = &io->buf[io->read];
        return size;
    } else {                    /* read and replenish */
        if (io->fd == -1) {     /* already closed, return what we've got */
            *buf = &io->buf[io->read];
            return rem;
        }

        memmove(io->buf, &io->buf[io->read], rem);
        io->fill -= io->read;
        io->read = 0;
        ssize_t read_sz = read(io->fd, &io->buf[io->fill], io->size - io->fill);
        if (read_sz < 0) { HEATSHRINK_ERR(1, "read"); }
        io->total += read_sz;
        if (read_sz == 0) {     /* EOF */
            if (close(io->fd) < 0) { HEATSHRINK_ERR(1, "close"); }
            io->fd = -1;
        }
        io->fill += read_sz;
        *buf = io->buf;
        return io->fill > size ? size : io->fill;
    }
}

/* Drop the oldest SIZE bytes from the buffer. Returns <0 on error. */
static int handle_drop(io_handle *io, size_t size) {
    LOG("@ drop %zd\n", size);
    if (io->read + size <= io->fill) {
        io->read += size;
    } else {
        return -1;
    }
    if (io->read == io->fill) {
        io->read = 0;
        io->fill = 0;
    }
    return 0;
}

/* Sink SIZE bytes from INPUT into the io handle. Returns the number of
 * bytes written, or -1 on error. */
static ssize_t handle_sink(io_handle *io, size_t size, uint8_t *input) {
    LOG("@ sink %zd\n", size);
    if (size > io->size) { return -1; }
    if (io->mode != IO_WRITE) { return -1; }

    if (io->fill + size > io->size) {
        ssize_t written = write(io->fd, io->buf, io->fill);
        LOG("@ flushing %zd, wrote %zd\n", io->fill, written);
        io->total += written;
        if (written == -1) { HEATSHRINK_ERR(1, "write"); }
        memmove(io->buf, &io->buf[written], io->fill - written);
        io->fill -= written;
    }
    memcpy(&io->buf[io->fill], input, size);
    io->fill += size;
    return size;
}

void handle_close(io_handle *io) {
    if (io->fd != -1) {
        if (io->mode == IO_WRITE) {
            ssize_t written = write(io->fd, io->buf, io->fill);
            io->total += written;
            LOG("@ close: flushing %zd, wrote %zd\n", io->fill, written);
            if (written == -1) { HEATSHRINK_ERR(1, "write"); }
        }
        close(io->fd);
        io->fd = -1;
    }
}

static void close_and_report(config *cfg) {
//    handle_close(cfg->in);
//    handle_close(cfg->out);
    if (cfg->verbose) { report(cfg); }
//    free(cfg->in);
//    free(cfg->out);
}

static int encoder_sink_read(config *cfg, heatshrink_encoder *hse,
        uint8_t *data, size_t data_sz) {
    size_t out_sz = 4096;
    uint8_t out_buf[out_sz];
    memset(out_buf, 0, out_sz);
    size_t sink_sz = 0;
    size_t poll_sz = 0;
    HSE_sink_res sres;
    HSE_poll_res pres;
    HSE_finish_res fres;
    io_handle *out = cfg->out;

    size_t sunk = 0;
    do {
        if (data_sz > 0) {
            sres = heatshrink_encoder_sink(hse, &data[sunk], data_sz - sunk, &sink_sz);
            if (sres < 0) { die("sink"); }
            sunk += sink_sz;
        }
        
        do {
            pres = heatshrink_encoder_poll(hse, out_buf, out_sz, &poll_sz);
            if (pres < 0) { die("poll"); }
            if (handle_sink(out, poll_sz, out_buf) < 0) die("handle_sink");
        } while (pres == HSER_POLL_MORE);
        
        if (poll_sz == 0 && data_sz == 0) {
            fres = heatshrink_encoder_finish(hse);
            if (fres < 0) { die("finish"); }
            if (fres == HSER_FINISH_DONE) { return 1; }
        }
    } while (sunk < data_sz);
    return 0;
}

static int encode(config *cfg) {
    uint8_t window_sz2 = cfg->window_sz2;
    size_t window_sz = 1 << window_sz2; 
    heatshrink_encoder *hse = heatshrink_encoder_alloc(window_sz2, cfg->lookahead_sz2);
    if (hse == NULL) { die("failed to init encoder: bad settings"); }
    ssize_t read_sz = 0;
    io_handle *in = cfg->in;

    /* Process input until end of stream */
    while (1) {
        uint8_t *input = NULL;
        read_sz = handle_read(in, window_sz, &input);
        if (input == NULL) {
            fprintf(stderr, "handle read failure\n");
            die("read");
        }
        if (read_sz < 0) { die("read"); }

        /* Pass read to encoder and check if input is fully processed. */
        if (encoder_sink_read(cfg, hse, input, read_sz)) break;

        if (handle_drop(in, read_sz) < 0) { die("drop"); }
    };
	
    if (read_sz == -1) { HEATSHRINK_ERR(1, "read"); }

    heatshrink_encoder_free(hse);
    close_and_report(cfg);
    return 0;
}
#define    BUF_SIZE         1024
static int encoder_sink_read_chuangmi(config *cfg, heatshrink_encoder *hse,
        uint8_t *data, size_t data_sz,  uint8_t *pdata_out, size_t *data_sz_out ) {
        
    size_t out_sz = *data_sz_out ;
//    uint8_t out_buf[out_sz];
//    memset(out_buf, 0, out_sz);
	cfg = cfg;
    size_t sink_sz = 0;
    size_t poll_sz = 0;
    HSE_sink_res sres;
    HSE_poll_res pres;
    HSE_finish_res fres;
//    io_handle *out = cfg->out;
//	size_t offset_out = 0;
    size_t sunk = 0;
//	printf("sssssssssssss=%d\n",data_sz);
//	int count = 0; 
    do {
//		printf("---------------->>count=%d\n", count++);
        if (data_sz > 0) {
            sres = heatshrink_encoder_sink(hse, &data[sunk], data_sz - sunk, &sink_sz);
            if (sres < 0) { die("sink"); }
            sunk += sink_sz;
        }
//        printf("sres=%d\n", sres);
        do {
            pres = heatshrink_encoder_poll(hse, pdata_out, BUF_SIZE, &poll_sz);
            if (pres < 0) { die("poll"); }
			*data_sz_out = poll_sz;
//            if (handle_sink(out, poll_sz, out_buf) < 0) die("handle_sink");
        } while (pres == HSER_POLL_MORE);
		
        if (poll_sz == 0 && data_sz == 0) {
            fres = heatshrink_encoder_finish(hse);
            if (fres < 0) { die("finish"); }
            if (fres == HSER_FINISH_DONE) { return 1; }
        }
    } while (sunk < data_sz);
    return 0;
}

static int decoder_sink_read_chuangmi(config *cfg, heatshrink_decoder *hsd,
        uint8_t *data, size_t data_sz,  uint8_t *pdata_out, size_t *data_sz_out) {
//    io_handle *out = cfg->out;
    size_t sink_sz = 0;
    size_t poll_sz = 0;
//    size_t out_sz_t = *data_sz_out;
//    uint8_t out_buf[out_sz];
//    memset(out_buf, 0, out_sz);
	cfg = cfg;

    HSD_sink_res sres;
    HSD_poll_res pres;
    HSD_finish_res fres;
//	printf("sssssssssssss=%d\n",data_sz);

    size_t sunk = 0;
    do {
        if (data_sz > 0) {
            sres = heatshrink_decoder_sink(hsd, &data[sunk], data_sz - sunk, &sink_sz);
            if (sres < 0) { die("sink"); }
            sunk += sink_sz;
        }
        do {
            pres = heatshrink_decoder_poll(hsd, pdata_out, BUF_SIZE, &poll_sz);
            if (pres < 0) { die("poll"); }
			*data_sz_out = poll_sz;
//            if (handle_sink(out, poll_sz, out_buf) < 0) die("handle_sink");
        } while (pres == HSDR_POLL_MORE);
        
        if (data_sz == 0 && poll_sz == 0) {
            fres = heatshrink_decoder_finish(hsd);
            if (fres < 0) { die("finish"); }
            if (fres == HSDR_FINISH_DONE) { return 1; }
        }
    } while (sunk < data_sz);

    return 0;
}

static int encode_chuangmi(config *cfg, uint8_t *input_arg , ssize_t read_sz_arg ,uint8_t *pdecode_out , size_t *pdecode_out_len) {
    uint8_t window_sz2 = cfg->window_sz2;
//    size_t window_sz = 1 << window_sz2; 
	
    heatshrink_encoder *hse = heatshrink_encoder_alloc(window_sz2, cfg->lookahead_sz2);
    if (hse == NULL) { die("failed to init encoder: bad settings"); }
	
    ssize_t read_sz = 0;
//    io_handle *in = cfg->in;
	cfg = cfg;

    /* Process input until end of stream */
	uint8_t *input = NULL;
	input = input_arg;
	read_sz = read_sz_arg;
	size_t  count_size = 0, tmp_ct = 0;
	uint8_t tmp_out[BUF_SIZE];
    while (1) {

//        read_sz = handle_read(in, window_sz, &input);
        if (input == NULL) {
            fprintf(stderr, "handle read failure\n");
            die("read");
        }
        if (read_sz < 0) { die("read"); }
		
        /* Pass read to encoder and check if input is fully processed. */
        if (encoder_sink_read_chuangmi(cfg, hse, input, read_sz, tmp_out, &tmp_ct))
		{
			break;
        }
		if (tmp_ct > 0)
		{
			 memcpy((pdecode_out+count_size),tmp_out, tmp_ct);
			 count_size += tmp_ct;   
		}
		
		printf("read_sz_arg_out=%d\n" ,(int)count_size);
		
//        if (handle_drop(in, read_sz) < 0) { die("drop"); }
		read_sz = 0;

    };
	*pdecode_out_len = count_size;

    if (read_sz == -1) { HEATSHRINK_ERR(1, "read"); }

    heatshrink_encoder_free(hse);
    close_and_report(cfg);
    return 0;
}


 int decode_chuangmi(config *cfg ,uint8_t *input_arg , ssize_t read_sz_arg ,uint8_t *pdecode_out , size_t *pdecode_out_len) {
    uint8_t window_sz2 = cfg->window_sz2;
//    size_t window_sz = 1 << window_sz2;
    size_t ibs = cfg->decoder_input_buffer_size;
    heatshrink_decoder *hsd = heatshrink_decoder_alloc(ibs,
        window_sz2, cfg->lookahead_sz2);
    if (hsd == NULL) { die("failed to init decoder"); }

    ssize_t read_sz = 0;

    io_handle *in = cfg->in;

    HSD_finish_res fres;
	uint8_t* input = input_arg;
	  read_sz = read_sz_arg;
	
	size_t  count_size = 0, tmp_ct = 0;
	uint8_t tmp_out[BUF_SIZE];

    /* Process input until end of stream */
    while (1) {
//        uint8_t *input = NULL;
//        read_sz = handle_read(in, window_sz, &input);
//        if (input == NULL) {
//            fprintf(stderr, "handle read failure\n");
//            die("read");
//        }
        if (read_sz == 0) {
            fres = heatshrink_decoder_finish(hsd);
            if (fres < 0) { die("finish"); }
            if (fres == HSDR_FINISH_DONE) break;
        } else if (read_sz < 0) {
            die("read");
        } else {
            if (decoder_sink_read_chuangmi(cfg, hsd, input, read_sz, tmp_out,&tmp_ct))
			{
				break;
			}
//			printf("tmp_ct=%d\n",tmp_ct);
			if (tmp_ct > 0)
			{
				 memcpy((pdecode_out + count_size), tmp_out, tmp_ct);
				 count_size += tmp_ct;   
			}
//			printf("tttt\n");
//            if (handle_drop(in, read_sz) < 0) { die("drop"); }
        }
		
		read_sz = 0;
    }
	*pdecode_out_len = count_size;
	
    if (read_sz == -1) { HEATSHRINK_ERR(1, "read"); }
        
    heatshrink_decoder_free(hsd);
    close_and_report(cfg);
    return 0;
}

static int decoder_sink_read(config *cfg, heatshrink_decoder *hsd,
        uint8_t *data, size_t data_sz) {
    io_handle *out = cfg->out;
    size_t sink_sz = 0;
    size_t poll_sz = 0;
    size_t out_sz = 4096;
    uint8_t out_buf[out_sz];
    memset(out_buf, 0, out_sz);

    HSD_sink_res sres;
    HSD_poll_res pres;
    HSD_finish_res fres;

    size_t sunk = 0;
    do {
        if (data_sz > 0) {
            sres = heatshrink_decoder_sink(hsd, &data[sunk], data_sz - sunk, &sink_sz);
            if (sres < 0) { die("sink"); }
            sunk += sink_sz;
        }

        do {
            pres = heatshrink_decoder_poll(hsd, out_buf, out_sz, &poll_sz);
            if (pres < 0) { die("poll"); }
            if (handle_sink(out, poll_sz, out_buf) < 0) die("handle_sink");
        } while (pres == HSDR_POLL_MORE);
        
        if (data_sz == 0 && poll_sz == 0) {
            fres = heatshrink_decoder_finish(hsd);
            if (fres < 0) { die("finish"); }
            if (fres == HSDR_FINISH_DONE) { return 1; }
        }
    } while (sunk < data_sz);

    return 0;
}

static int decode(config *cfg) {
    uint8_t window_sz2 = cfg->window_sz2;
    size_t window_sz = 1 << window_sz2;
    size_t ibs = cfg->decoder_input_buffer_size;
    heatshrink_decoder *hsd = heatshrink_decoder_alloc(ibs,
        window_sz2, cfg->lookahead_sz2);
    if (hsd == NULL) { die("failed to init decoder"); }

    ssize_t read_sz = 0;

    io_handle *in = cfg->in;

    HSD_finish_res fres;

    /* Process input until end of stream */
    while (1) {
        uint8_t *input = NULL;
        read_sz = handle_read(in, window_sz, &input);
        if (input == NULL) {
            fprintf(stderr, "handle read failure\n");
            die("read");
        }
        if (read_sz == 0) {
            fres = heatshrink_decoder_finish(hsd);
            if (fres < 0) { die("finish"); }
            if (fres == HSDR_FINISH_DONE) break;
        } else if (read_sz < 0) {
            die("read");
        } else {
            if (decoder_sink_read(cfg, hsd, input, read_sz)) { break; }
            if (handle_drop(in, read_sz) < 0) { die("drop"); }
        }
    }
    if (read_sz == -1) { HEATSHRINK_ERR(1, "read"); }
        
    heatshrink_decoder_free(hsd);
    close_and_report(cfg);
    return 0;
}

static void report(config *cfg) {
    size_t inb = cfg->in->total;
    size_t outb = cfg->out->total;
    fprintf(cfg->out->fd == STDOUT_FILENO ? stderr : stdout,
        "%s %0.2f %%\t %zd -> %zd (-w %u -l %u)\n",
        cfg->in_fname, 100.0 - (100.0 * outb) / inb, inb, outb,
        cfg->window_sz2, cfg->lookahead_sz2);
}

static void proc_args(config *cfg, int argc, char **argv) {
    cfg->window_sz2 = DEF_WINDOW_SZ2;
    cfg->lookahead_sz2 = DEF_LOOKAHEAD_SZ2;
    cfg->buffer_size = DEF_BUFFER_SIZE;
    cfg->decoder_input_buffer_size = DEF_DECODER_INPUT_BUFFER_SIZE;
    cfg->cmd = OP_ENC;
    cfg->verbose = 0;
    cfg->in_fname = "-";
    cfg->out_fname = "-";

    int a = 0;
    while ((a = getopt(argc, argv, "hedi:w:l:v")) != -1) {
        switch (a) {
        case 'h':               /* help */
            usage();
        case 'e':               /* encode */
            cfg->cmd = OP_ENC; break;
        case 'd':               /* decode */
            cfg->cmd = OP_DEC; break;
        case 'i':               /* input buffer size */
            cfg->decoder_input_buffer_size = atoi(optarg);
            break;
        case 'w':               /* window bits */
            cfg->window_sz2 = atoi(optarg);
            break;
        case 'l':               /* lookahead bits */
            cfg->lookahead_sz2 = atoi(optarg);
            break;
        case 'v':               /* verbosity++ */
            cfg->verbose++;
            break;
        case '?':               /* unknown argument */
        default:
            usage();
        }
    }
    argc -= optind;
    argv += optind;
    if (argc > 0) {
        cfg->in_fname = argv[0];
        argc--;
        argv++;
    }
    if (argc > 0) { cfg->out_fname = argv[0]; }
}

int decode_chuangmi_app(uint8_t *encode_char, ssize_t encode_char_len , uint8_t *encode_out_char, size_t *encode_out_charlen)
{
		 config cfg;
		memset(&cfg, 0, sizeof(cfg));
	//----------------test------------------------
	cfg.window_sz2 = DEF_WINDOW_SZ2;
	cfg.lookahead_sz2 = DEF_LOOKAHEAD_SZ2;
	cfg.buffer_size = DEF_BUFFER_SIZE;
	cfg.decoder_input_buffer_size = DEF_DECODER_INPUT_BUFFER_SIZE;
	cfg.cmd = OP_DEC;
	cfg.verbose = 0;
	cfg.in_fname = "-";
	cfg.out_fname = "-";
	
//	memset(encode_out_char, 0x00, sizeof(encode_out_char));


	decode_chuangmi(&cfg, (uint8_t *)encode_char, encode_char_len, (uint8_t *)encode_out_char, encode_out_charlen);

	printf("decode_out_charlen= %d\n", *encode_out_charlen);
	size_t ulI;
	for ( ulI = 0; ulI < *encode_out_charlen; ulI++ )
	{
	    printf("%c", encode_out_char[ulI]);
	}

	printf("\n");
	return 0;

}

int main(int argc, char **argv) {
    config cfg;
    memset(&cfg, 0, sizeof(cfg));

//----------------test------------------------
//	cfg.window_sz2 = DEF_WINDOW_SZ2;
//	cfg.lookahead_sz2 = DEF_LOOKAHEAD_SZ2;
//	cfg.buffer_size = DEF_BUFFER_SIZE;
//	cfg.decoder_input_buffer_size = DEF_DECODER_INPUT_BUFFER_SIZE;
//	cfg.cmd = OP_ENC;
//	cfg.verbose = 0;
//	cfg.in_fname = "-";
//	cfg.out_fname = "-";
//
//	
//	char encode_char[100] ;
//	char encode_out_char[100];
//	size_t  encode_out_charlen = 0;
//
//
//	memset(encode_char, 0x31, sizeof(encode_char));
//	memset(encode_out_char, 0x00, sizeof(encode_out_char));
//
//	encode_chuangmi(&cfg, (uint8_t *)encode_char, sizeof(encode_char), (uint8_t *)encode_out_char, &encode_out_charlen);
//
//	printf("encode_out_charlen= %d\n", (int)encode_out_charlen);
//	size_t ulI;
//	for ( ulI = 0; ulI < encode_out_charlen; ulI++ )
//	{
//	    printf("0x%x,", encode_out_char[ulI]);
//	}
//
//	printf("\n");
//	return 0;
//----------------test------------------------

	
	cfg.window_sz2 = DEF_WINDOW_SZ2;
	cfg.lookahead_sz2 = DEF_LOOKAHEAD_SZ2;
	cfg.buffer_size = DEF_BUFFER_SIZE;
	cfg.decoder_input_buffer_size = DEF_DECODER_INPUT_BUFFER_SIZE;
	cfg.cmd = OP_DEC;
	cfg.verbose = 0;
	cfg.in_fname = "-";
	cfg.out_fname = "-";

	
	unsigned char encode_char[]={
		 153, 77, 166, 115, 137, 100, 226, 115, 52, 150, 77, 38, 160, 6, 64, 22, 64, 15, 115, 137, 196, 204, 1, 226, 99, 51, 154, 76, 192, 32, 166, 160, 65, 64, 56, 64, 15, 128, 47, 224, 161, 224, 14, 115, 112, 15, 137, 204, 194, 100, 9, 229, 49, 3, 127, 3, 255, 2, 250, 152, 129, 123, 0, 250, 129, 249, 205, 38, 97, 30, 161, 80, 224, 175, 96, 63, 97, 161, 160, 190, 211, 57, 156, 178, 109, 57, 152, 206, 102, 114, 193, 3, 96, 218, 241, 3, 249, 160, 129, 248, 121, 248, 113, 248, 11, 248, 23, 248, 129, 216, 19, 248, 31, 248, 43, 168, 23, 172, 192, 58, 240, 56, 212, 37, 204, 64, 252, 64, 236, 86, 228, 9, 236, 64, 214, 97, 49, 156, 73, 132, 15, 194, 11, 8, 31, 196, 15, 194, 175, 192, 31, 192, 31, 200, 31, 200, 31, 196, 143, 195, 46, 7, 255, 199, 255, 192, 191, 199, 255, 199, 255, 199, 255, 194, 171, 199, 255, 230, 115, 64, 91, 48, 40, 240, 251, 241, 255, 240, 251, 240, 31, 49, 255, 240, 39, 240, 251, 240, 251, 241, 247, 179, 188, 144, 251, 242, 200, 241, 247, 249, 168
		
	};
	
	char encode_out_char[2000];
	size_t  encode_out_charlen = 0;

	memset(encode_out_char, 0, sizeof(encode_out_char));
	decode_chuangmi_app(encode_char, 211, encode_out_char, &encode_out_charlen);
	
	
	printf("%s\n",encode_out_char);
	
	
	return 0;


//----------------test------------------------

	proc_args(&cfg, argc, argv);

    if (0 == strcmp(cfg.in_fname, cfg.out_fname)
        && (0 != strcmp("-", cfg.in_fname))) {
        fprintf(stderr, "Refusing to overwrite file '%s' with itself.\n", cfg.in_fname);
        exit(1);
    }

    cfg.in = handle_open(cfg.in_fname, IO_READ, cfg.buffer_size);
    if (cfg.in == NULL) { die("Failed to open input file for read"); }
    cfg.out = handle_open(cfg.out_fname, IO_WRITE, cfg.buffer_size);
    if (cfg.out == NULL) { die("Failed to open output file for write"); }

#if _WIN32
    /*
     * On Windows, stdin and stdout default to text mode. Switch them to
     * binary mode before sending data through them.
     */
    _setmode(STDOUT_FILENO, O_BINARY);
    _setmode(STDIN_FILENO, O_BINARY);
#endif

    if (cfg.cmd == OP_ENC) {
        return encode(&cfg);
    } else if (cfg.cmd == OP_DEC) {
        return decode(&cfg);
    } else {
        usage();
    }
}
