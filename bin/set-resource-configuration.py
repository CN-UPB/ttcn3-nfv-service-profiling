#!/usr/bin/env python
#
# Copyright 2018 Christian Dröge <mail@cdroege.de>
#
# All rights reserved. This program and the accompanying materials are
# made available under the terms of the Eclipse Public License v2.0 which
# accompanies this distribution and is available at
#
# http://www.eclipse.org/legal/epl-v20.html

import sys
import ruamel.yaml
import io

def main(filepath, vnf, vcpus, mem, storage, datacenter=None):

    with open(filepath) as stream:
        try:
            yaml_dict = (ruamel.yaml.load(stream, Loader=ruamel.yaml.RoundTripLoader, preserve_quotes=True))
            if(datacenter != None):
                yaml_dict["dc"] = datacenter
            else:
                del yaml_dict["dc"]
            vdus = yaml_dict["virtual_deployment_units"]

            # Note: We assume one vdu
            vdus[0]["resource_requirements"]["cpu"]["vcpus"] = int(float(vcpus))
            vdus[0]["resource_requirements"]["memory"]["size"] = int(float(mem))
            vdus[0]["resource_requirements"]["storage"]["size"] = int(float(storage))
            vdus[0]["resource_requirements"]["memory"]["size_unit"] = "MB"
            vdus[0]["resource_requirements"]["storage"]["size_unit"] = "GB"
            with io.open(filepath, 'w', encoding='utf8') as outfile:
                ruamel.yaml.dump(yaml_dict, outfile, Dumper=ruamel.yaml.RoundTripDumper, allow_unicode=True)
            return 0

        except ruamel.yaml.YAMLError as exc:
            print(exc)
            return 1

        return 1

if __name__ == "__main__":
    if(len(sys.argv) == 6):
        sys.exit(main(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5]))
    elif(len(sys.argv) == 7):
        sys.exit(main(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5], sys.argv[6]))
    else:
        print("Wrong number of parameters")
        sys.exit(1)


