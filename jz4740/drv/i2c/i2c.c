//#include <regs.h>
//#include <ops.h>
#include <jz4740.h>

//#define DEVCLK	3686400
#define DEVCLK	11200000

/* I2C protocol */
#define I2C_READ	1
#define I2C_WRITE	0

#define TIMEOUT         1000

/* error code */
#define ETIMEDOUT	1
#define ENODEV		2

/*
 * I2C bus protocol basic routines
 */
static int i2c_put_data(unsigned char data)
{
	unsigned int timeout = TIMEOUT*10;
	__i2c_write(data);
	__i2c_set_drf();
	while (__i2c_check_drf() != 0);
	while (!__i2c_transmit_ended());
	while (!__i2c_received_ack() && timeout)
		timeout--;
	if (timeout)
		return 0;
	else
		return -ETIMEDOUT;
}

static int i2c_get_data(unsigned char *data, int ack)
{
	int timeout = TIMEOUT*10;

	if (!ack)
		__i2c_send_nack();
	else
		__i2c_send_ack();

	while (__i2c_check_drf() == 0 && timeout)
		timeout--;

	if (timeout) {
		if (!ack)
			__i2c_send_stop();
		*data = __i2c_read();
		__i2c_clear_drf();
		return 0;
	} else
		return -ETIMEDOUT;
}

/*
 * I2C interface
 */
int i2c_read(unsigned char device, unsigned char *buf,
	     unsigned char offset, int count)
{
	unsigned char *ptr = buf;
	int cnt = count;
	int timeout = 5;

	__i2c_set_clk(DEVCLK, 10000); /* default 10 KHz */
	__i2c_enable();

L_try_again:

	if (timeout < 0)
		goto L_timeout;

	__i2c_send_nack();	/* Master does not send ACK, slave sends it */
	__i2c_send_start();
	if (i2c_put_data( (device << 1) | I2C_WRITE ) < 0)
		goto device_werr;
	if (i2c_put_data(offset) < 0)
		goto offset_err;
//	__i2c_send_stop();// add for sccb of camera
	__i2c_send_start();
	if (i2c_put_data( (device << 1) | I2C_READ ) < 0)
		goto device_rerr;
	__i2c_send_ack();	/* Master sends ACK for continue reading */
	while (cnt) {
		if (cnt == 1) {
			if (i2c_get_data(buf, 0) < 0)
				break;
		} else {
			if (i2c_get_data(buf, 1) < 0)
				break;
		}
		cnt--;
		buf++;
	}

	__i2c_send_stop();

	udelay(300); /* wait for STOP goes over. */
	__i2c_disable();

	return count - cnt;

 device_rerr:
 device_werr:
	printf("ERROR: No I2C device 0x%2x.\n", device);
 offset_err:
	timeout --;
	__i2c_send_stop();
	goto L_try_again;

L_timeout:
	printf("Read I2C device 0x%2x failed.\n", device);
	__i2c_send_stop();
	udelay(300); /* wait for STOP goes over. */
	__i2c_disable();
	return -ENODEV;
}

int i2c_write(unsigned char device, unsigned char *buf,
	      unsigned char offset, int count)
{
	int cnt = count;
	int cnt_in_pg;
	int timeout = 5;
	unsigned char *tmpbuf;
	unsigned char tmpaddr;
	__i2c_set_clk(DEVCLK, 10000); /* default 10 KHz */
	__i2c_enable();

	__i2c_send_nack();	/* Master does not send ACK, slave sends it */
 W_try_again:
	if (timeout < 0)
		goto W_timeout;

	cnt = count;
	tmpbuf = (unsigned char *)buf;
	tmpaddr = offset;

 start_write_page:
	cnt_in_pg = 0;
 	__i2c_send_start();
	if (i2c_put_data( (device << 1) | I2C_WRITE ) < 0)
		goto device_err;
	if (i2c_put_data(tmpaddr) < 0)
		goto offset_err;
	while (cnt) { 
		if (++cnt_in_pg > 8) {
			__i2c_send_stop();
			mdelay(1);
			tmpaddr += 8;
			goto start_write_page;
		}
		if (i2c_put_data(*tmpbuf) < 0)
			break;
		cnt--;
		tmpbuf++;
	}
	__i2c_send_stop();
	udelay(300); /* wait for STOP goes over. */
	__i2c_disable();
	return count - cnt;
 device_err:
	printf("ERROR: No I2C device 0x%2x.\n", device);
 offset_err: 
	printf("ERROR: I2C device Address error.\n", offset);
	timeout--;
	__i2c_send_stop();
	goto W_try_again;

 W_timeout:
	printf("Write I2C device 0x%2x failed.\n", device);
	__i2c_send_stop();
	udelay(300); /* wait for STOP goes over. */
	__i2c_disable();
	return -ENODEV;
}


