#ifndef TUNE_H
#define TUNE_H

void tune()
{
    /* PSM2 */
    setenv("HFI_NO_CPUAFFINITY",        "1",       0);
    setenv("PSM2_SHAREDCONTEXTS",       "0",       0);
    setenv("PSM2_MEMORY",               "large",   0);
    setenv("PSM2_TID_SENDSESSIONS_MAX", "2097152", 0);
    setenv("PSM2_RCVTHREAD",            "0",       0);
    setenv("PSM2_DEVICES",              "shm,hfi", 0);

    /* OFI/PSM2 */
    setenv("FI_PSM2_NAME_SERVER",        "0",      0);
    setenv("FI_PSM2_PROG_THREAD",        "0",      0);

    setenv("I_MPI_COLL_INTRANODE", "pt2pt", 0);
}

#endif /* TUNE_H */
