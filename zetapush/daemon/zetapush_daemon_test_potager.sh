if [ $# -eq 0 ];  then
	echo "No arguments supplied"
	echo "Usage: ./zetapush_daemon.sh configuration_file "
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

content='{\"temp\":\"-10\",\"lum\":\"20\",\"hum\":\"30\"}'
echo $content

wget --header='Content-Type: application/json' --header='X-Authorization: {"data":{"password":"'$pass'", "login":"'$pass'"}, "resource":"'$resource'", "type":"'$businessID'.'$authenticationID'.simple"}' --post-data '{"name":"'$macroName'", "parameters":{"data":"'$content'"}}' 'http://free-1.zpush.io/str/restd/'$businessID'/'$macroID'/call' 



