#include <iostream>
#include <msquic.h>
int main()
{
    const QUIC_API_TABLE * ptr = {nullptr};
    if (auto status = MsQuicOpen2(&ptr); QUIC_FAILED(status))
    {
        std::cout << "MsQuicOpen2 failed: " << status << std::endl;
        return 1;
    }
    std::cout << "MsQuicOpen2 succeeded";
    MsQuicClose(ptr);
    return 0;
}