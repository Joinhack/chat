#ifndef DUMP_H
#define DUMP_H

#ifdef __cplusplus
extern "C" {
#endif

int dump_save(server *svr);

int dump_load(server *svr);

#ifdef __cplusplus
}
#endif

#endif /**end dump head define*/
