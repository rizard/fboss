#include "TapIntf.h"

TapIntf::TapIntf(const std::string &iface_name, const unsigned short iface_index) : 
	name_(iface_name), fd_(0), index_(iface_index)
{
	bringUpIface();
}

TapIntf::~TapIntf()
{
	takeDownIface();
}

int TapIntf::bringUpIface()
{
	int rc;
	int flags;
	struct ifreq ifr;

	/* Check if we're already running */
	if (fd_ != 0)
	{
		std::cout << "Tried to initialize " << name_ << ", but it was already running" << std::endl;
		return -1;
	}

	if ((fd_ = open("/dev/net/tun", O_RDWR)) < 0)
	{
		std::cout << "Could not open file descriptor for " << name_ << ". open() returned " << fd_ << std::endl;
		rc = fd_;
		fd_ = 0;
		return rc;
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	size_t len = std::min(name_.size(), sizeof(ifr.ifr_name));
	memmove(ifr.ifr_name, name_.c_str(), len);

	if ((rc = ioctl(fd_, TUNSETIFF, (void *) &ifr)) < 0)
	{
		std::string err;
		if (errno == EBADF)
		{
			err = std::string("Invalid file descriptor, errno=") + std::to_string(errno);	
		}
		else if (errno == EFAULT)
		{
			err = std::string("Argument references inaccessible memory area, errno=") + std::to_string(errno);
		}
		else if (errno == EINVAL)
		{
			err = std::string("File descriptor not associated with a character special device, errno=") + std::to_string(errno);
		}
		else if (errno == ENOTTY)
		{
			err = std::string("Request invalid for object type referenced by file descriptor, errno=") + std::to_string(errno);
		}
		else if (errno == EPERM)
		{
			err = std::string("Insufficient permissions to create tap interface, errno=") + std::to_string(errno);
		}
		else if (errno == EBUSY)
		{
			err = std::string("Device is busy, errno=") + std::to_string(errno);
		}
		else 
		{
			err = std::string("Unknown errno=") + std::to_string(errno);
		}
		std::cout << "Could not open interface " << std::string(ifr.ifr_name) << " for packet RX/TX. ioctl() returned " << rc << ". " << err << std::endl;
		close(fd_);
		fd_ = 0;
		return rc;
	}
	
	if ((flags = fcntl(fd_, F_GETFL, 0)) < 0)
	{
		std::cout << "Could not get flags for interface " << name_ << ". fcntl() returned " << flags << std::endl;
		close(fd_);
		fd_ = 0;
		return flags;
	}

	flags |= O_NONBLOCK;
	
	if ((rc = fcntl(fd_, F_SETFL, flags)) < 0)
	{
		std::cout << "Could not set flags (nonblocking) for interface " << name_ << ". fcntl() returned " << rc << std::endl;
		close(fd_);
		fd_ = 0;
		return rc;
	}

	return 0;
}

int TapIntf::takeDownIface()
{
	int rc;

	if (fd_ == 0)
	{
		std::cout << "Interface " << name_ << " already removed" << std::endl;
		return -1;
	}
	else if ((rc = close(fd_)) < 0)
	{
		std::cout << "Error closing file descriptor for interface " << name_ << std::endl;
	}
	else
	{
		std::cout << "Removed interface " << name_ << std::endl;
		fd_ = 0;
	}
}
