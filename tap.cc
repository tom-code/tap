
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>


#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/socket.h>
#include <sys/ioctl.h>



static int tap_alloc(const char *dev, int flags) {
  struct ifreq ifr;
  int fd, err;
  const char *clonedev = "/dev/net/tun";

  fd = open(clonedev, O_RDWR);
  if (fd < 0) {
    return -1;
  }

  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = flags;
  strncpy(ifr.ifr_name, dev, IFNAMSIZ);

  err = ioctl(fd, TUNSETIFF, (void *) &ifr);
  if ( err < 0 ) {
    close(fd);
    return -1;
  }

  return fd;
}

static void set_up(const char *device) {
  int s = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

  struct ifreq  ifr;
  memset(&ifr, 0, sizeof(ifr));
  strcpy(ifr.ifr_name, device);
  if (ioctl(s, SIOCGIFFLAGS, &ifr) == -1) {
    printf("can't get interface flags %s\n", device);
    return;
  }

  ifr.ifr_flags |= IFF_UP | IFF_LOWER_UP | IFF_NOARP;
  if (ioctl(s, SIOCSIFFLAGS, &ifr) == -1) {
    printf("can't switch raw interface %s to up\n", device);
    return;
  }
  close(s);
}

static void iface_set_mtu(const char *device, int mtu)
{
  int s = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  strcpy(ifr.ifr_name, device);
  ifr.ifr_mtu = mtu;

  if (ioctl(s, SIOCSIFMTU, &ifr) == -1) {
  }
  close(s);
}



int main(int argc, char **argv)
{
  int tunfd = tap_alloc("tap1", IFF_TAP | IFF_NO_PI);
  if (tunfd == -1) {
    printf("can't create tap device\n");
    return 0;
  }
  iface_set_mtu("tap1", 60000);
  set_up("tap1");

  while (true) {
    unsigned char buf[100*1024];
    int r = read(tunfd, buf, sizeof(buf));
    if (r < 1) break;
    for (int i=0; i<r; i++) {
      printf("%02x ", buf[i]);
    }
    unsigned char message[]= {0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2,    0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa1,    0x00, 0x00,    0x00, 0x00, 0x00, 0x00};
    write(tunfd, message, sizeof(message));
    printf("\n");
  }
  
  printf("device ready; press enter to quit\n");
  char a[100];
  fgets(a, sizeof(a), stdin);
  close(tunfd);
}



