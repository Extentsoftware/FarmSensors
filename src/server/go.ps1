az account set --subscription 73a97781-98ea-4939-aa9d-f4c6233b011e
az aks get-credentials --resource-group Bongo --name Bongo
cd 02-bridge
docker build . -t gluteusmaximus/mqttbridge
docker push  gluteusmaximus/mqttbridge
cd ..
cd azure/kubectl
kubectl apply -f mqttbongo.yaml
cd ../..
