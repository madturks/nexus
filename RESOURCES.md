

# Movement system demo and related articles:
https://www.gabrielgambetta.com/client-side-prediction-live-demo.html

# Movement system optimization paper (RING):
[text](https://www.cs.princeton.edu/~funk/symp95.pdf),

# A rookie's approach to the MMORPG development:
[text](https://wirepair.org/2023/06/29/so-you-want-to-build-an-mmorpg-server/)


# Quic superpowers blog post
[text](https://quic.video/blog/quic-powers/)

# Edgeshark

```
wget -q --no-cache -O - \
  https://github.com/siemens/edgeshark/raw/main/deployments/wget/docker-compose-localhost.yaml \
  | DOCKER_DEFAULT_PLATFORM= docker compose -f - up
```

Also install this plugin:

https://github.com/siemens/cshargextcap/releases

then navigate to: http://localhost:5001/


/*
    Generally, MsQuic creates multiple threads to parallelize work, and
   therefore will make parallel/overlapping upcalls to the application, but
   not for the same connection. All upcalls to the app for a single
   connection and all child streams are always delivered serially. This is
   not to say, though, it will always be on the same thread. MsQuic does
   support the ability to shuffle connections around to better balance the
   load.
*/

export LTTNG_UST_DEBUG=1
export LTTNG_UST_REGISTER_TIMEOUT=-1
sudo apt-get install liblttng-ust-dev liblttng-ctl-dev

