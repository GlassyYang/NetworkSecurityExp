import javax.swing.*;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;
import javax.swing.table.DefaultTableModel;
import java.io.IOException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class MainUI {
    public static void main(String[] args) {
        JFrame frame = new JFrame("MainUI");
        frame.setLocation(200, 200);
        frame.setSize(600, 610);
        frame.setContentPane(new MainUI().mainPanel);
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setVisible(true);
    }

    private JPanel mainPanel;
    private JTextField host;
    private JButton scan;
    private JButton cancel;
    private JTable table;
    private JProgressBar progress;
    private JTextField from;
    private JTextField to;
    private JTextArea console;
    private JSlider threads;
    private JLabel threadNum;

    private MainUI(){
        threads.addChangeListener(new ChangeListener() {
            @Override
            public void stateChanged(ChangeEvent e) {
                threadNum.setText(Integer.toString(threads.getValue()));
            }
        });
    }

    private void createUIComponents() {
        //添加控制台输出
        console = new JTextArea();
        //设置进度条
        progress = new JProgressBar();
        progress.setIndeterminate(true);
        //跟table有关的设置
        table = new JTable();
        // TODO: place custom component creation code here
        String[] title = {"端口", "类型", "状态"};
        DefaultTableModel model = new DefaultTableModel(null, title);
        table.setModel(model);
        progress = new JProgressBar();
        //跟TextField有关的设置
        from = new JTextField();
        to = new JTextField();
        host = new JTextField();
        threads = new JSlider();
        threads.setMaximum(101);
        threads.setMinimum(1);
        threads.setValue(20);
        threads.setPaintLabels(true);
        threads.setMajorTickSpacing(10);
        threads.setMinorTickSpacing(5);
        //启用线程池
        ExecutorService executer = Executors.newCachedThreadPool();
        scan = new JButton();
        cancel = new JButton();
        cancel.setEnabled(false);
        Scan[] scannerArr = {null};
        scan.addActionListener((e)->{
                //同一时间只能有一个扫描任务
                if(scannerArr[0] != null){
                    console.append("当前已有扫描任务在运行，请等待任务完成或取消任务。\n");
                    return;
                }
                //进行扫描之前的准备
                String fromStr = from.getText();
                String toStr = to.getText();
                String hostStr = host.getText();
                if(fromStr.isEmpty() || toStr.isEmpty()){
                    console.append("请输入开始和结束端口号！\n");
                    return;
                }
                if(hostStr.isEmpty()){
                    console.append("请输入IP地址或域名！\n");
                    return;
                }
                int fromInt, toInt;
                try{
                    fromInt = Integer.parseInt(fromStr);
                    toInt = Integer.parseInt(toStr);
                }catch(NumberFormatException except){
                    console.append("端口号输入不正确！\n");
                    return;
                }
                if(fromInt > toInt || fromInt < 0 || toInt > 65535){
                    console.append("端口号输入不正确！\n");
                    return;
                }
                Scan scanner;
                try{
                    scanner = Scan.getInstance(fromInt, toInt, hostStr);
                }catch(IOException except){
                    console.append("域名输入不正确！（请注意是否输入了空格\n");
                    return;
                }
                scannerArr[0] = scanner;
                cancel.setEnabled(true);
                threads.setEnabled(false);
                executer.execute(()->{
                        String res = scanner.beginScan(model, progress, threads.getValue());
                        console.append(res);
                        cancel.setEnabled(false);
                        threads.setEnabled(true);
                        scannerArr[0] = null;
                    });
                console.append("开始扫描...\n");
            });
        cancel.addActionListener((e)-> {
                //添加取消有关的代码;
                if(scannerArr[0] == null){
                    console.append("出现内部错误...\n");
                    return;
                }
                console.append("canceling...\n");
                scannerArr[0].cancel = true;
            });
    }
}
