import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.io.IOException;
import java.net.*;
public class Scan {

    private int from;
    private int to;
    private InetAddress addr;
    private int width;
    public boolean cancel;
    private Integer finished;
    private Scan(int from, int to, InetAddress addr) {
        this.addr = addr;
        this.from = from;
        this.to = to;
        width = to - from + 1;
        finished = 0;
    }

    public String beginScan(DefaultTableModel model, JProgressBar progressBar, final int threadsNum){
        cancel = false;
        progressBar.setMinimum(0);
        progressBar.setMaximum(width - 1);
        progressBar.setValue(0);
        Thread[] threads = new Thread[threadsNum];
        Thread monitor;
        monitor = new Thread(()->{
            do{
                progressBar.setValue(finished);
                try {
                    Thread.sleep(500);
                } catch (InterruptedException e) {
                }
                if(cancel){
                    finished = 0;
                    return;
                }
            }while(finished < width);
            finished = 0;
        });
        monitor.start();
        Thread thread;
        int index = 0;
        for(int i = from; i <= to; i++){
            //判断用户是否取消扫描
            if(cancel){
                return "扫描被取消。\n";
            }
            //开启线程，执行扫描过程;
            final int temp = i;
            thread = new Thread(()->{
                try {
                    Socket socket = new Socket(addr, temp);
                    socket.close();
                }catch(IOException e){
                    return;
                }finally {
                    synchronized (finished){
                        finished++;
                    }
                }
                synchronized (model){
                    model.addRow(new String[]{Integer.toString(temp), "TCP", "OPENED"});
                }
            });
            if(index < threadsNum){
                thread.start();
                threads[index++] = thread;
            }else{
                int insert = -1;
                do{
                    for(int j = 0; j < threadsNum; j++){
                        if(!threads[j].isAlive()){
                            insert = j;
                            break;
                        }
                    }
                    if(cancel){
                        return "扫描被取消。\n";
                    }
                    try {
                        Thread.sleep(500);
                    } catch (InterruptedException e) {

                    }
                }while(insert == -1);
                thread.start();
                threads[insert] = thread;
            }
        }
        boolean notExit = true;
        while(notExit){
            notExit = false;
            for(int i = 0; i < Math.min(threadsNum, width); i++) {
                if(threads[i].isAlive()){
                    notExit = true;
                }
            }
        }
        assert monitor.isAlive() == false;
        return "扫描完成。\n";
    }

    public static Scan getInstance(int from, int to, String addr) throws UnknownHostException{
        InetAddress host = InetAddress.getByName(addr);
        if(from > to || from < 0 || to > 65535){
            return null;
        }
        return new Scan(from, to, host);
    }
}
