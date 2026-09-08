#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "rknn_api.h"

static double now_ms(void){ struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts); return ts.tv_sec*1000.0 + ts.tv_nsec/1e6; }

int main(int argc, char **argv){
    const char *path = argc > 1 ? argv[1] : "model.rknn";
    rknn_context ctx = 0;
    int ret;

    ret = rknn_init(&ctx, path, 0, 0, NULL);
    printf("rknn_init ret = %d\n", ret);
    if (ret != RKNN_SUCC) return ret;

    rknn_sdk_version ver;
    ret = rknn_query(ctx, RKNN_QUERY_SDK_VERSION, &ver, sizeof(ver));
    printf("rknn_query sdk_version ret=%d\n  api_version=%s\n  drv_version=%s\n", ret, ver.api_version, ver.drv_version);

    rknn_input_output_num io;
    memset(&io,0,sizeof(io));
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io, sizeof(io));
    printf("rknn_query in/out num ret=%d in=%u out=%u\n", ret, io.n_input, io.n_output);
    if (ret != RKNN_SUCC) goto out;

    rknn_tensor_attr in_attr;
    memset(&in_attr,0,sizeof(in_attr));
    in_attr.index = 0;
    ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &in_attr, sizeof(in_attr));
    printf("rknn_query input attr ret=%d dims=%d %d %d %d fmt=%d type=%d size=%u\n",
        ret, in_attr.dims[0], in_attr.dims[1], in_attr.dims[2], in_attr.dims[3],
        in_attr.fmt, in_attr.type, in_attr.n_elems);

    unsigned int h = in_attr.dims[1], w = in_attr.dims[2], c = in_attr.dims[3];
    if (in_attr.fmt == RKNN_TENSOR_NHWC) { h = in_attr.dims[1]; w = in_attr.dims[2]; c = in_attr.dims[3]; }
    else { c = in_attr.dims[1]; h = in_attr.dims[2]; w = in_attr.dims[3]; }
    unsigned long long bytes = (unsigned long long)h * w * c;
    unsigned char *img = calloc(bytes ? bytes : 1, 1);
    if (!img) { printf("calloc fail\n"); ret = -1; goto out; }

    rknn_input inputs[1];
    memset(inputs,0,sizeof(inputs));
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_UINT8;
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    inputs[0].buf = img;
    inputs[0].size = bytes;
    ret = rknn_inputs_set(ctx, 1, inputs);
    printf("rknn_inputs_set ret=%d\n", ret);

    double t0 = now_ms();
    ret = rknn_run(ctx, NULL);
    double dt = now_ms() - t0;
    printf("rknn_run ret=%d  elapsed=%.1f ms\n", ret, dt);

    if (ret == RKNN_SUCC) {
        rknn_output outputs[1];
        memset(outputs,0,sizeof(outputs));
        outputs[0].want_float = 1;
        outputs[0].index = 0;
        ret = rknn_outputs_get(ctx, 1, outputs, NULL);
        printf("rknn_outputs_get ret=%d buf@%p size=%u\n", ret, outputs[0].buf, outputs[0].size);
        if (ret == RKNN_SUCC && outputs[0].buf) {
            float *f = (float*)outputs[0].buf;
            unsigned int n = outputs[0].size / sizeof(float);
            float mx = -1e30; int mi = -1;
            for (unsigned int i = 0; i < n; i++) { if (f[i] > mx) { mx = f[i]; mi = (int)i; } }
            printf("outputs[0] floats=%u maxidx=%d maxval=%f\n", n, mi, mx);
            rknn_outputs_release(ctx, 1, outputs);
        }
    }
    free(img);
out:
    rknn_destroy(ctx);
    return ret == RKNN_SUCC ? 0 : 1;
}
