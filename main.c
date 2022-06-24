#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include "errno.h"
#include "gumbo.h"
#include "gumbo-get-element-by-id.c/src/get-element-by-id.h"

#define MAX_LEN 65536


int ProblemNum;

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written;
    written = fwrite(ptr, size, nmemb, stream);
    return written;
}

int gethtml() {
    // 리턴값 1: curl 다운로드 오류
    // 문제 url 완성
    char url[256];
    sprintf(url, "https://www.acmicpc.net/problem/%d", ProblemNum);

    // url의 페이지 다운로드
    char outf[64];
    printf("다운로드 및 분석 중: %s\n", url);

    CURL *curl = curl_easy_init();

    if (curl) {
        mkdir("ProblemPage", 0777);
        sprintf(outf, "ProblemPage/%d", ProblemNum);
        mkdir(outf, 0777);
        sprintf(outf, "ProblemPage/%d/problem.html", ProblemNum);
        FILE *fp = fopen(outf, "w");

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_USERAGENT,
                         "user-agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/88.0.4324.182 Safari/537.36");

        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        fclose(fp);
    }


    // 다운받은 페이지 파싱
    FILE *fp = fopen(outf, "r");
    char doc[MAX_LEN];
    fread(doc, 1, MAX_LEN, fp);
    fclose(fp);

    GumboNode *gumbo = gumbo_parse(doc)->root;
    GumboNode *sampleGumbo;

    char *example;
    int exampleNum;
    char elemId[32];
    _Bool isInput = true, isEnd = false;
    unsigned int size;

    for (exampleNum = 0; 1; exampleNum++) {
        for (int i = 0; i <= 1; i++) {

            if (isInput) {
                sprintf(elemId, "sample-input-%d", exampleNum + 1);
            } else {
                sprintf(elemId, "sample-output-%d", exampleNum + 1);
            }

            sampleGumbo = gumbo_get_element_by_id(elemId, gumbo);
            if (sampleGumbo == NULL) {
                isEnd = true;
                break;
            }

            size = (sampleGumbo->v.element.end_pos.offset - sampleGumbo->v.element.start_pos.offset) -
                   sampleGumbo->v.element.original_tag.length - sampleGumbo->v.element.original_end_tag.length + 5;


            example = malloc(size + 32);
            strncpy(example,
                    sampleGumbo->v.element.original_tag.data + sampleGumbo->v.element.original_tag.length,
                    size);

            sprintf(outf, "ProblemPage/%d/%d.%s", ProblemNum, exampleNum + 1, isInput ? "in" : "out");

            fp = fopen(outf, "w");
            fputs(example, fp);


            isInput = !isInput;
            fclose(fp);
            free(example);
        }
        if (isEnd) {
            char pInfo[4];
            sprintf(pInfo, "%d", exampleNum);
            sprintf(outf, "ProblemPage/%d/info.txt", ProblemNum);
            fp = fopen(outf, "w");
            fwrite(pInfo, 1, strlen(pInfo), fp);
            fclose(fp);
            break;
        }
    }

    return 0;
}


int test(char argv[128]) {
    FILE *fp;
    int exampleNum;
    char fpath[32], cmd[128];
    _Bool isInput = true;
    char in[MAX_LEN];
    int totalExNum;


    sprintf(fpath, "ProblemPage/%d/info.txt", ProblemNum);
    char buf[8];
    fp = fopen(fpath, "r");
    if (fp == NULL) {
        gethtml();
        fp = fopen(fpath, "r");
    }

    fread(buf, 1, 8, fp);
    fclose(fp);
    totalExNum = atoi(buf);
    if (!totalExNum) {
        printf("입출력이 없습니다. 문제 번호를 제대로 입력했는지 확인하십시오");
        return -1;
    }


    for (exampleNum = 0; exampleNum < totalExNum; exampleNum++) {

        // 테스트 출력
        sprintf(fpath, "ProblemPage/%d/%d.in", ProblemNum, exampleNum + 1);
        sprintf(cmd, "cat %s | %s > ./ProblemPage/%d/%d.usrout", fpath, argv, ProblemNum, exampleNum + 1);

        system(cmd);


        // 확인
        sprintf(fpath, "ProblemPage/%d/%d.out", ProblemNum, exampleNum + 1);
        sprintf(cmd, "diff  %s ./ProblemPage/%d/%d.usrout", fpath, ProblemNum, exampleNum + 1);
        if (system(cmd)) {
            printf("%d번: 틀렸습니다\n", exampleNum + 1);
            return 1;
        } else {
            printf("%d번: 맞았습니다\n", exampleNum + 1);
            return 0;
        }
    }
    return -1;
}

int main(int argc, char *argv[]) {
    if (argc < 3) printf("인수가 적습니다.\n ./BaekJoonTester {문제 번호} {프로그램 실행 명령}");
    ProblemNum = atoi(argv[1]);

    char executable[128] = {0,};
    for (int i = 2; i < argc; i++) {
        strcat(executable, argv[i]);
        strcat(executable, " ");
    }
    return test(executable);
}

