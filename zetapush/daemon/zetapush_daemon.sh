if [ $# -eq 0 ];  then
	echo "No arguments supplied"
	echo "Usage: ./zetapush_daemon.sh configuration_file "
	exit 0
fi

source $1
echo $user
echo $pass
echo $resource
echo $businessID
echo $authenticationID
echo $macroID
echo $macroName
echo $conectedObject
echo $loop

while true; do

	content=$(wget $conectedObject -q -t 1 --timeout=120 -O -)
	echo "$content"

	if [ "$content" != "" ]; then
 	
		wget --header='Content-Type: application/json' --header='X-Authorization: {"data":{"password":"'$pass'", "login":"'$pass'"}, "resource":"'$resource'", "type":"'$businessID'.'$authenticationID'.simple"}' --post-data '{"name":"'$macroName'", "parameters":{"data":"'$content'"}}' 'http://free-1.zpush.io/str/restd/'$businessID'/'$macroID'/call' -qO- &> /dev/null
	else
		echo "could not connect with the object"
	fi
	
	sleep $loop
done


