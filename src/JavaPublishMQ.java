package javapublishmq;

import java.nio.ByteBuffer;
import org.fusesource.mqtt.client.FutureConnection;
import org.fusesource.mqtt.client.MQTT;
import org.fusesource.mqtt.client.QoS;

public class JavaPublishMQ {

    private final static String CONNECTION_STRING = "tcp://120.26.86.124:1883";
    private final static boolean CLEAN_START = true;
    private final static short KEEP_ALIVE = 30;// 低耗网络，但是又需要及时获取数据，心跳30s      
    private final static String topic = "/0123456789abcdefg/lpcloud/candata";
    public final static long RECONNECTION_ATTEMPT_MAX = 6;
    public final static long RECONNECTION_DELAY = 2000;
    public final static int SEND_BUFFER_SIZE = 2 * 1024 * 1024;//发送最大缓冲为2M  

    public static void main(String[] args) {
        MQTT mqtt = new MQTT();
        try {
            //设置服务端的ip  
            mqtt.setHost(CONNECTION_STRING);

            mqtt.setUserName("easydarwin");
            mqtt.setPassword("123456");
            mqtt.setClientId("Java publisher");
            //连接前清空会话信息  
            mqtt.setCleanSession(CLEAN_START);
            //设置重新连接的次数  
            mqtt.setReconnectAttemptsMax(RECONNECTION_ATTEMPT_MAX);
            //设置重连的间隔时间  
            mqtt.setReconnectDelay(RECONNECTION_DELAY);
            //设置心跳时间  
            mqtt.setKeepAlive(KEEP_ALIVE);
            //设置缓冲的大小  
            mqtt.setSendBufferSize(SEND_BUFFER_SIZE);

            final FutureConnection futureConnection = mqtt.futureConnection();
            futureConnection.connect();
            byte payload[] = {0, 1};
            Integer sigTypeCode = 123456;
            futureConnection.publish(topic, genPL(), QoS.AT_LEAST_ONCE, false);
            //futureConnection.publish(topic, "hello".getBytes(), QoS.AT_LEAST_ONCE, false);
            Thread.sleep(1000);

            futureConnection.disconnect();

            System.out.println("done.");
        } catch (Exception e) {
            e.printStackTrace();
        }

    }

    public static byte[] IntToBytes(int value) {
        byte[] byteArray = new byte[4];
        byteArray[0] = (byte) ((value >> 24) & 0xFF);
        byteArray[1] = (byte) ((value >> 16) & 0xFF);
        byteArray[2] = (byte) ((value >> 8) & 0xFF);
        byteArray[3] = (byte) (value & 0xFF);
        //System.out.println(src.toString());
        return byteArray;
    }

    public static byte[] IntToBytes(int value, byte[] byteArray, int offset) {
        byteArray[offset] = (byte) ((value >> 24) & 0xFF);
        byteArray[offset + 1] = (byte) ((value >> 16) & 0xFF);
        byteArray[offset + 2] = (byte) ((value >> 8) & 0xFF);
        byteArray[offset + 3] = (byte) (value & 0xFF);
        //System.out.println(src.toString());
        return byteArray;
    }

    public static byte[] ShortToBytes(short value, byte[] byteArray, int offset) {
        byteArray[offset] = (byte) ((value >> 8) & 0xFF);
        byteArray[offset + 1] = (byte) (value & 0xFF);
        //System.out.println(src.toString());
        return byteArray;
    }

// data format expected like | lenghtOfSignalValue(1) | signalTypeCode(4) | signalValue(lenghtOfSignalValue) | time(8) | ... |    
    public static void AddFloat(ByteBuffer buf, int signalCode, float data) {
        buf.put((byte) 4);
        buf.putInt(signalCode);
        buf.putFloat(data);
        buf.putLong(System.currentTimeMillis() / 1000);
    }

    public static void AddInt(ByteBuffer buf, int signalCode, int data) {
        buf.put((byte) 4);
        buf.putInt(signalCode);
        buf.putInt(data);
        buf.putLong(System.currentTimeMillis() / 1000);
    }

    public static void AddShort(ByteBuffer buf, int signalCode, short data) {
        buf.put((byte) 2);
        buf.putInt(signalCode);
        buf.putShort(data);
        buf.putLong(System.currentTimeMillis() / 1000);
    }

    public static void AddByte(ByteBuffer buf, int signalCode, byte data) {
        buf.put((byte) 1);
        buf.putInt(signalCode);
        buf.put(data);
        buf.putLong(System.currentTimeMillis() / 1000);
    }

    public static byte[] genPL() {
        byte[] src = new byte[256];
        ByteBuffer payload = ByteBuffer.wrap(src);

        // 扭矩 * 10 + 20000
//        AddFloat(payload, 1020108, (float)-1980.1);
//        AddFloat(payload, 1020108, (float)980.1);
//        AddFloat(payload, 1020108, (float)-1210);
//        AddFloat(payload, 1020108, (float)10);
//        
//        // 转速 + 20000
//        AddShort(payload, 1020101, (short)-20000);
//        AddShort(payload, 1020101, (short)10);
//        
//        // 电压 *10
//        AddFloat(payload, 1030201, (float)3.1);
//        AddFloat(payload, 1030201, (float)3);
//        
//        // 驱动电机温度 +40
//        AddShort(payload, 1020201, (short)-12);
//        AddByte(payload, 1020201, (byte)120);
//        AddShort(payload, 1020201, (short)210);
        
        // 最高、低温度
        AddByte(payload, 1030401, (byte) 210);
        AddByte(payload, 1030402, (byte) 20);
//        
//        // 通用报警标志
//        // AddInt(payload, 8, 4294967295); 超出范围没法测
//
//        // 经纬度
//        AddInt(payload, 2, 180000000);
//        AddInt(payload, 3, -180000000);

        return src;

    }
}
