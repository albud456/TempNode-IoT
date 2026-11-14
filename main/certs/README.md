# AWS IoT credentials
 
This folder holds the credentials the firmware uses to connect to AWS IoT Core. They are embedded into the firmware image at build time by `main/CMakeLists.txt`.
 
**Nothing in this folder except this README is committed to git.** Never commit the private key.
 
## Required files
 
The file names must match exactly. They are referenced in `main/CMakeLists.txt` with `target_add_binary_data(... TEXT)`:
 
| File | What it is | Where it comes from |
|---|---|---|
| `aws_root_ca_pem` | Amazon root CA, used to verify the AWS IoT endpoint | Public. Download it from the AWS IoT Core developer guide (see "Server authentication"). Amazon Root CA 1 is the usual choice |
| `certificate_pem_crt` | This device's X.509 certificate | AWS IoT Core console, Security, Certificates |
| `private_pem_key` | The device's private key | Downloadable only once, when the certificate is created |
 
AWS downloads use names like `AmazonRootCA1.pem`, `<id>-certificate.pem.crt`, and `<id>-private.pem.key`. Rename each file to the name in the table:
 
```bash
cp ~/Downloads/AmazonRootCA1.pem          main/certs/aws_root_ca_pem
cp ~/Downloads/<id>-certificate.pem.crt   main/certs/certificate_pem_crt
cp ~/Downloads/<id>-private.pem.key       main/certs/private_pem_key
chmod 600 main/certs/private_pem_key
```
 
## If the private key is lost
 
AWS cannot give you the key again. In the AWS IoT Core console:
 
1. Create a new certificate and download its files.
2. Activate it, and attach it to the same Thing and IoT policy.
3. Deactivate or delete the old certificate.
## Keeping secrets out of git
 
The repository `.gitignore` should contain:
 
```gitignore
main/certs/*
!main/certs/README.md
```
 
Check that it works before adding any credentials:
 
```bash
git check-ignore -v main/certs/private_pem_key
git status --short        # no credential files should be listed
```
 
## Build-only placeholders (no AWS connection)
 
To compile without real credentials, for example in CI, create placeholder files:
 
```bash
for f in aws_root_ca_pem certificate_pem_crt private_pem_key; do
  printf -- '-----BEGIN PLACEHOLDER-----\n-----END PLACEHOLDER-----\n' > main/certs/$f
done
```
 
A firmware built this way cannot connect to AWS. Replace the placeholders with the real files before flashing. Real credentials should only come from a secrets store, never from the repository.
