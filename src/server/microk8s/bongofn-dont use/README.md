# Preparation
1. Use vscode and install kubernetes extension.
2. gain access to the microk8s kubernetes on the server by merging in the kube config specified at the end of https://docs.google.com/document/d/1f2cjbFj58xJU4cpUW-iIIwjFUqZD7-h_PRJAWnzgMjw/edit?tab=t.0#heading=h.r7dq9nrddfi3

# To upgrade the backend server
if neccessary, rebuild and publish a new image of bongo.webserver. Once updates, take a note of the image tag and update bongofn-deployment.yaml accordingly.

within the BongoFms repo run the following to upgrade the backend..

`helm upgrade bongofn bongofn`

You should be able to access the backend and inspect endpoints via `https://bongofn.vestrong.eu/swagger/index.html`