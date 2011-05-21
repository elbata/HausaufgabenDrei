#!/bin/sh

# Para ver los valores de los parametros 
# ir al web: http://www.linuxfoundation.org/en/Net:Netem

# Defino variables del script
INTERFACE="lo"
DELAY="100ms 10ms" 	# la variable DELAY debe contener pegada la unidad ms
LOSS="10%" 		# la variable LOSS debe contener pegado %
REORDER="25% 50%"	# la variable REORDER debe contener pegado %
DUPLICATE="4%"		# la variable DUPLICATE debe contener pegado %

down_proc(){

# Inicializo el tc
tc qdisc del dev $INTERFACE root netem 

echo "Ha quedado tc sin configurar"
tc -s qdisc ls dev $INTERFACE
}

stat_proc(){

echo "La configuracion es:"
tc -s qdisc ls dev $INTERFACE
}

up_proc(){
# Inicializo el tc
tc qdisc del dev $INTERFACE root netem 


if [ "A$DELAY" != "A" ]
then
  #tc qdisc add dev $INTERFACE root netem delay $DELAY
  PARAMETROS="delay $DELAY $PARAMETROS"
fi
if [ "A$LOSS" != "A" ]
then
  #tc qdisc add dev $INTERFACE root netem loss $LOSS
  PARAMETROS="loss $LOSS $PARAMETROS"
fi
if [ "A$REORDER" != "A" ]
then
  #tc qdisc add dev  $INTERFACE root netem reorder $REORDER 
  PARAMETROS="reorder $REORDER $PARAMETROS"
fi
if [ "A$DUPLICATE" != "A" ]
then
  #tc qdisc add dev $INTERFACE root netem duplicate $DUPLICATE
  PARAMETROS="duplicate $DUPLICATE $PARAMETROS"
fi

echo "Configuro red con los siguientes parametros:"
echo "-->tc qdisc add dev $INTERFACE root netem $PARAMETROS"
tc qdisc add dev $INTERFACE root netem $PARAMETROS


echo "Ha quedado tc configurado:"
tc -s qdisc ls dev $INTERFACE
}


case "$1" in
  up|UP)
    up_proc
    ;;
  down|DOWN)
    down_proc
    ;;
  stat|STAT)
    stat_proc
    ;;
  *)
    echo "Usage: $0 {up|down|stat}" >&2
    exit 3
    ;;
esac
