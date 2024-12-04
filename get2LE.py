import requests
import sys

noradID = sys.argv[1]
#norad_number = "INSERT_NORAD_NUMBER_HERE"
norad_number = noradID

#open file to write to
f = open('sat2LE.txt','w')

# Construct the URL for the TLE file
base_url = "https://celestrak.org/NORAD/elements/"
url = base_url + "gp.php?CATNR="+ norad_number

# Send a GET request to the URL and retrieve the TLE file contents
response = requests.get(url)
tle_file_contents = response.text


# Print the TLE file contents
print(tle_file_contents)
f.write(tle_file_contents)
f.close()
