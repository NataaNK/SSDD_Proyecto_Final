import binascii
import ipaddress


addr = ipaddress.ip_address('176.58.10.138')

print(addr)
print('   IP version:', addr.version)
print('   is private:', addr.is_private)
print('  packed form:', binascii.hexlify(addr.packed))
print('      integer:', int(addr))
print()
