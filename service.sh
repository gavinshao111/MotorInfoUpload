target=./motorinfoupload
startup_script=startup.sh


# start
start(){
    if [ $(ps -ef | grep "$target" | grep -v "grep" | wc -l) -eq 1 ]; then
        echo "Service is already running ..."
        exit 1
    fi

    sh $startup_script
    echo "Service startup " $(date)
}
#stop
stop(){
        
    if [ $(ps -ef | grep "$target" | grep -v "grep" | wc -l) -eq 1 ];then
        kill $(ps -ef | grep "$target" | grep -v "grep" | awk '{print $2}')
        if [ $? -eq 0 ]; then
            echo killed
        else
            echo kill fail
            exit 1
        fi	
        echo "Service stopped."			
    else
        echo "Service is not running."
    fi
 
}
#restart
restart(){
    stop
    sleep 1
    start
}

usage(){
    echo "Usage:{start|stop|restart|status}"
}
status(){
    if [ $(ps -ef | grep "$target" | grep -v "grep" | wc -l) -eq 1 ];then
        echo "Service is running ..."
    else
        echo "Service is not running."
    fi
}
case $1 in
start)
    start
    ;;
stop)
    stop
    ;;
restart)
    restart
    ;;
status)
    status
    ;;
*)
    usage
    exit 7
    ;;
esac

