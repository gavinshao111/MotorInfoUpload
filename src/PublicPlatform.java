/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package publicplatform;

import java.io.IOException;
import java.io.InputStream;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.logging.Level;
import java.util.logging.Logger;
import sun.misc.Signal;
import sun.misc.SignalHandler;

/**
 *
 * @author 10256
 */
public class PublicPlatform implements SignalHandler {

    private ByteBuffer m_dataUnitBuf;
    private int m_dataUnitLength;
    private boolean m_quit;
    Socket m_socket = null;
    InputStream m_is = null;
    PrintWriter m_pw = null;
    ServerSocket m_serverSocket = null;
    short m_cmdId;

    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        try {
            PublicPlatform publicPlatform = new PublicPlatform();
            publicPlatform.onService();

        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public PublicPlatform() {
        m_dataUnitBuf = ByteBuffer.allocate(51200);
        m_dataUnitLength = 0;
        m_quit = false;
        m_cmdId = -1;
    }

    public void close() throws IOException {
        if (m_pw != null) {
            m_pw.close();
        }
        if (m_is != null) {
            m_is.close();
        }
        if (m_socket != null && m_socket.isConnected()) {
            m_socket.close();
        }
        if (m_serverSocket != null && !m_serverSocket.isClosed()) {
            m_socket.close();
        }

    }

    public void onService() throws IOException, Exception {
        try {
            PublicPlatform publicPlatformHandler = new PublicPlatform();
            Signal.handle(new Signal("TERM"), publicPlatformHandler);
            Signal.handle(new Signal("INT"), publicPlatformHandler);

//            byte tmp[] = new byte[512];
            m_serverSocket = new ServerSocket(1234);

            int i = 0;
            for (; !m_quit; i++) {
                System.out.print("\n" + i + ". server is listening... ");
                m_socket = m_serverSocket.accept();

                System.out.println("connection setup.");
                m_is = m_socket.getInputStream();
                m_pw = new PrintWriter(m_socket.getOutputStream());
                try {

                    readHeaderAndDataUnit();
                    if (m_cmdId != 5) {
                        m_pw.write("you need login first");
                        m_pw.flush();
                        continue;
                    }

                    m_is.read();
//                System.out.println("login BCC code: " + );
                    analyzeDataUnit();
                    m_pw.write("ok");
                    m_pw.flush();

                    // read car signal data
                    readHeaderAndDataUnit();
                    m_is.read();
//                System.out.println("car signal data BCC code: " + );
                    analyzeDataUnit();

                    m_pw.write("ok");
                    m_pw.flush();
                    // read lotout data
                    readHeaderAndDataUnit();
                    m_is.read();
//                System.out.println("logout BCC code: " + );
                    analyzeDataUnit();
                } catch (Exception e) {
                    m_pw.write("fail");
                    m_pw.flush();
                    throw e;
                } finally {
                    close();
                }
            }
        } catch (IOException | IllegalArgumentException e) {
            throw e;
        } finally {
            close();
        }

    }

    public void readHeaderAndDataUnit() throws Exception {
        m_dataUnitBuf.clear();
        if (m_is.read() != (int) '#' || m_is.read() != (int) '#') {
            throw new Exception("first 2 bytes expected ##");
        }
        m_cmdId = (short) m_is.read();
        short responseFlag = (short) m_is.read();

//        System.out.println("\ncmdid = " + m_cmdId);
//        System.out.println("responseFlag = " + responseFlag);
        byte vin[] = new byte[18];
        m_is.read(vin, 0, vin.length - 1);
//        System.out.println("vin = " + new String(vin));

        m_is.read();
//        System.out.println("encryptionAlgorithm = " + );

        m_dataUnitLength = (m_is.read() << 8) + m_is.read();
//        System.out.println("dataUnitLength = " + m_dataUnitLength);

        int curByteRead = m_is.read(m_dataUnitBuf.array(), 0, m_dataUnitLength);
        if (curByteRead != m_dataUnitLength) {
            throw new Exception(m_dataUnitLength + " bytes of data unit expected to read, but " + curByteRead + " bytes read actually.");
        }

        m_dataUnitBuf.position(curByteRead);
        m_dataUnitBuf.flip();
    }

    public void analyzeDataUnit() throws Exception {
        if (m_dataUnitBuf.remaining() == 0) {
            throw new Exception("data is empty");
        }

        Calendar calendar = Calendar.getInstance();
        int yy = m_dataUnitBuf.get() + 2000;
        int MM = m_dataUnitBuf.get() - 1;
        int dd = m_dataUnitBuf.get();
        int hh = m_dataUnitBuf.get();
        int mm = m_dataUnitBuf.get();
        int ss = m_dataUnitBuf.get();
        calendar.set(yy, MM, dd, hh, mm, ss);
//        System.out.println("\ntime = " + new SimpleDateFormat("yyyy-MM-dd hh:mm:ss").format(calendar.getTime()));
        System.out.println("time = " + yy + "-" + MM + "-" + dd + " " + hh + ":" + mm + ":" + ss);

        switch (m_cmdId) {
            case 5: // login
                parseLogin();
                break;
            case 2: // realtime data
            case 3: // reissue data
                parseRealtimeOrReissueData();
                break;
            case 6: // logout
                parseLogout();
                break;
            default:
                throw new Exception("illegal cmdId: " + m_cmdId);
        }

    }

    public void parseLogin() {
//        System.out.println("\nlogin data:");
        byte username[] = new byte[13];
        byte password[] = new byte[21];

        m_dataUnitBuf.getShort();
//        System.out.println("serialNumber = " + );
        m_dataUnitBuf.get(username, 0, username.length - 1);
        m_dataUnitBuf.get(password, 0, password.length - 1);
//        System.out.println("username = " + new String(username));
//        System.out.println("password = " + new String(password));
        m_dataUnitBuf.get();
//        System.out.println("encryptionAlgorithm = " + );
    }

    public void parseLogout() {
//        System.out.println("\nlogout data:");
        m_dataUnitBuf.getShort();
//        System.out.println("serialNumber = " + );
    }

    public void parseRealtimeOrReissueData() throws Exception {
//        System.out.println("\ncar signal data:");
        byte infoTypeCode;
        for (; m_dataUnitBuf.remaining() > 0;) {
            infoTypeCode = m_dataUnitBuf.get();
            switch (infoTypeCode) {
                case 1: {// CBV
                    System.out.println("CBV_vehicleStatus = " + m_dataUnitBuf.get());
                    System.out.println("CBV_chargeStatus = " + m_dataUnitBuf.get());
                    System.out.println("CBV_runningMode = " + m_dataUnitBuf.get());
                    System.out.println("CBV_speed = " + m_dataUnitBuf.getShort());
                    System.out.println("CBV_mileages = " + m_dataUnitBuf.getInt());
                    System.out.println("CBV_totalVoltage = " + m_dataUnitBuf.getShort());
                    System.out.println("CBV_totalCurrent = " + m_dataUnitBuf.getShort());
                    System.out.println("CBV_soc = " + m_dataUnitBuf.get());
                    System.out.println("CBV_dcdcStatus = " + m_dataUnitBuf.get());
                    System.out.println("CBV_gear = " + m_dataUnitBuf.get());
                    System.out.println("CBV_insulationResistance = " + (m_dataUnitBuf.getShort() & 0xffff));
                    System.out.println("CBV_reverse = " + m_dataUnitBuf.getShort());
                    break;
                }
                case 2: {// DM
                    System.out.println("DM_Num = " + m_dataUnitBuf.get());
                    System.out.println("DM_sequenceCode = " + m_dataUnitBuf.get());
                    System.out.println("DM_status = " + m_dataUnitBuf.get());
                    System.out.println("DM_controllerTemperature = " + (m_dataUnitBuf.get() & 0xff));
                    System.out.println("DM_rotationlSpeed = " + m_dataUnitBuf.getShort());
                    System.out.println("DM_torque = " + m_dataUnitBuf.getShort());
                    System.out.println("DM_temperature = " + (m_dataUnitBuf.get() & 0xff));
                    System.out.println("DM_controllerInputVoltage = " + (m_dataUnitBuf.getShort() & 0xffff));
                    System.out.println("DM_controllerDCBusCurrent = " + m_dataUnitBuf.getShort());
                    break;
                }
                case 5: {// L
                    System.out.println("L_status = " + m_dataUnitBuf.get());
                    System.out.println("L_longitudeAbs = " + m_dataUnitBuf.getInt());
                    System.out.println("L_latitudeAbs = " + m_dataUnitBuf.getInt());
                    break;
                }
                case 6: {// EV
                    System.out.println("EV_maxVoltageSysCode = " + m_dataUnitBuf.get());
                    System.out.println("EV_maxVoltageMonomerCode = " + (m_dataUnitBuf.get() & 0xff));
                    System.out.println("EV_maxVoltage = " + m_dataUnitBuf.getShort());
                    System.out.println("EV_minVoltageSysCode = " + m_dataUnitBuf.get());
                    System.out.println("EV_minVoltageMonomerCode = " + (m_dataUnitBuf.get() & 0xff));
                    System.out.println("EV_minVoltage = " + m_dataUnitBuf.getShort());
                    System.out.println("EV_maxTemperatureSysCode = " + m_dataUnitBuf.get());
                    System.out.println("EV_maxTemperatureProbeCode = " + (m_dataUnitBuf.get() & 0xff));
                    System.out.println("EV_maxTemperature = " + (m_dataUnitBuf.get() & 0xff));
                    System.out.println("EV_minTemperatureSysCode = " + m_dataUnitBuf.get());
                    System.out.println("EV_minTemperatureProbeCode = " + (m_dataUnitBuf.get() & 0xff));
                    System.out.println("EV_minTemperature = " + (m_dataUnitBuf.get() & 0xff));
                    break;
                }
                case 7: {// A
                    System.out.println("A_highestAlarmLevel = " + m_dataUnitBuf.get());
                    long generalAlarmSigns = m_dataUnitBuf.getInt() & 0xffffffffL;
                    System.out.println("A_generalAlarmSigns = " + generalAlarmSigns);
                    break;
                }
                default:
                    throw new Exception("illegal infomatoion type: " + infoTypeCode);
            }
        }
    }

    @Override
    public void handle(Signal signal) {
        System.out.println("receive " + signal);
        m_quit = true;
        try {
            close();
        } catch (IOException ex) {
            Logger.getLogger(PublicPlatform.class.getName()).log(Level.SEVERE, null, ex);
            ex.printStackTrace();
        }
        System.exit(0);
    }
}
