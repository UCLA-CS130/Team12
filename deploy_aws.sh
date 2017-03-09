echo save image
docker save -o httpserver.tar httpserver

echo 'Copy tar to AWS'
scp -i plantver.pem httpserver.tar ubuntu@52.53.150.174:/home/ubuntu

echo 'SSHing into AWS'
ssh -i plantver.pem ubuntu@52.53.150.174 << delimiter

    echo 'Loading the image into docker'
    docker load -i httpserver.tar

    echo 'Killing existing running containers'
    docker kill $(docker ps -q)

    echo 'Running the server'
    docker run --rm -t -p 2020:2020 httpserver

delimiter